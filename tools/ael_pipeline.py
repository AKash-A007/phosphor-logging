#!/usr/bin/env python3
"""
ael_pipeline.py - AEL Multi-Pass Event Validation Pipeline

Pass 1 : Walk PDI *.events.yaml  → discover all events + object_path params + primary check
Pass 2 : Read event_afid_map.yaml → associate events with AFIDs
Pass 3 : Validate everything      → stale events, invalid AFIDs, missing primary params
Output : CSV report + summary

Usage:
    python3 ael_pipeline.py \
        --pdi-dir   ./phosphor-dbus-interfaces/yaml/xyz/openbmc_project \
        --afid-map  ./yaml/com/amd/ael/event_afid_map.yaml \
        --registry  ./yaml/com/amd/ael/afid_registry.yaml \
        --out       ./ael_report.csv
"""

import argparse
import csv
import sys
from pathlib import Path
from dataclasses import dataclass, field

try:
    import yaml
except ImportError:
    print("[ERROR] PyYAML not installed. Run: pip install pyyaml")
    sys.exit(1)

# ─────────────────────────────────────────────────────────────────────────────
# Terminal colors
# ─────────────────────────────────────────────────────────────────────────────

import os
_COLOR = hasattr(sys.stdout, "isatty") and sys.stdout.isatty()

def _c(code, t): return f"\033[{code}m{t}\033[0m" if _COLOR else t
def red(t):    return _c("31;1", t)
def green(t):  return _c("32;1", t)
def yellow(t): return _c("33;1", t)
def cyan(t):   return _c("36;1", t)
def bold(t):   return _c("1", t)

def section(title):
    bar = "─" * 64
    print(f"\n{bold(bar)}\n  {bold(title)}\n{bold(bar)}")

def info(m):  print(f"  {cyan('INFO')}   {m}")
def ok(m):    print(f"  {green('OK')}     {m}")
def warn(m):  print(f"  {yellow('WARN')}   {m}")
def err(m):   print(f"  {red('ERROR')}  {m}")


# ─────────────────────────────────────────────────────────────────────────────
# Data model
# ─────────────────────────────────────────────────────────────────────────────

@dataclass
class ParamInfo:
    name:    str
    type:    str
    primary: bool

@dataclass
class EventInfo:
    event_id:       str
    kind:           str          # error | event
    severity:       str
    redfish_mapping: str
    params:         list[ParamInfo] = field(default_factory=list)

    # filled in Pass 2
    afid:           str = ""

    # filled in Pass 3
    valid:          bool = True
    issues:         list[str] = field(default_factory=list)

    @property
    def object_path_params(self) -> list[ParamInfo]:
        return [p for p in self.params if p.type == "object_path"]

    @property
    def primary_params(self) -> list[ParamInfo]:
        return [p for p in self.params if p.primary]

    @property
    def has_primary(self) -> bool:
        return len(self.primary_params) > 0

    @property
    def has_object_path(self) -> bool:
        return len(self.object_path_params) > 0


# ─────────────────────────────────────────────────────────────────────────────
# Pass 1 — PDI Discovery
# ─────────────────────────────────────────────────────────────────────────────

def pass1_discover(pdi_dir: str, amd_dir: str = "") -> dict[str, EventInfo]:
    """
    Pass 1: Walk PDI tree (and optional AMD-local tree), extract all events.
    Returns: { event_id -> EventInfo }
    """
    section("Pass 1 \u2014 PDI Event Discovery")

    pdi_path = Path(pdi_dir).resolve()
    if not pdi_path.exists():
        err(f"PDI directory not found: {pdi_dir}")
        sys.exit(1)

    yaml_files = sorted(pdi_path.rglob("*.events.yaml"))
    if not yaml_files:
        err(f"No *.events.yaml files found under: {pdi_dir}")
        sys.exit(1)

    info(f"Scanning {len(yaml_files)} *.events.yaml file(s)...")

    events: dict[str, EventInfo] = {}

    def _scan_dir(base_path: Path, namespace_root: str):
        """Scan a directory tree of *.events.yaml files."""
        for yf in sorted(base_path.rglob("*.events.yaml")):
            try:
                data = yaml.safe_load(open(yf))
            except Exception as e:
                warn(f"Could not read {yf.name}: {e}")
                continue

            if not isinstance(data, dict):
                continue

            # Build namespace from relative path
            rel = yf.relative_to(base_path)
            rel_parts = list(rel.parts)
            stem = rel_parts[-1].replace(".events.yaml", "")
            ns_parts = rel_parts[:-1] + [stem]
            namespace = namespace_root + "." + ".".join(ns_parts)

            # Determine format
            has_list_format = any(
                isinstance(data.get(k), list) and data.get(k) and
                isinstance(data[k][0], dict) and "name" in data[k][0]
                for k in ("events", "errors")
                if data.get(k)
            )

            entries_to_process = []  # list of (event_id, metadata_list)

            if has_list_format:
                for kind in ("errors", "events"):
                    for entry in data.get(kind, []) or []:
                        if not isinstance(entry, dict) or "name" not in entry:
                            continue
                        event_id = f"{namespace}.{entry['name']}"
                        entries_to_process.append(
                            (event_id, entry.get("metadata", []) or []))
            elif "metadata" in data:
                # AMD single-event top-level format (one file = one event).
                # Convention: Cper.events.yaml -> event name CperReported
                event_name = stem + "Reported" if not stem.endswith(
                    ("Reported", "Failed", "Restored", "Fault", "Changed")) \
                    else stem
                event_id = f"{namespace}.{event_name}"
                entries_to_process.append((event_id, data.get("metadata", []) or []))

            for event_id, meta_list in entries_to_process:
                params = []
                for p in meta_list:
                    if not isinstance(p, dict):
                        continue
                    primary_raw = p.get("primary", False)
                    primary = primary_raw if isinstance(primary_raw, bool) \
                              else str(primary_raw).lower() == "true"
                    params.append(ParamInfo(
                        name=p.get("name", ""),
                        type=p.get("type", ""),
                        primary=primary,
                    ))

                ev = EventInfo(
                    event_id=event_id,
                    kind="event",
                    severity=data.get("severity", ""),
                    redfish_mapping=data.get("redfish-mapping", ""),
                    params=params,
                )
                events[event_id] = ev

                obj_paths = [p.name for p in ev.object_path_params]
                primaries = [p.name for p in ev.primary_params]
                info(f"{event_id}")
                info(f"  object_path params : {obj_paths if obj_paths else '\u2014'}")
                info(f"  primary params     : {primaries if primaries else '\u2014'}")

    # Scan PDI (xyz.openbmc_project.*)
    _scan_dir(pdi_path, "xyz.openbmc_project")

    # Optionally scan AMD-local events (com.amd.*)
    if amd_dir:
        amd_path = Path(amd_dir).resolve()
        if amd_path.exists():
            info(f"Scanning AMD-local events under: {amd_dir}")
            _scan_dir(amd_path, "com.amd")
        else:
            warn(f"AMD event dir not found (skipping): {amd_dir}")

    print()
    ok(f"Discovered {len(events)} event(s) from PDI + AMD")
    return events


# ─────────────────────────────────────────────────────────────────────────────
# Pass 2 — AFID Association
# ─────────────────────────────────────────────────────────────────────────────

def pass2_associate(
    events: dict[str, EventInfo],
    afid_map_path: str,
    registry_path: str,
) -> tuple[dict[str, str], dict[str, str]]:
    """
    Pass 2: Load AFID map + registry, stamp AFID onto each EventInfo.
    Returns: (afid_map, registry)
    """
    section("Pass 2 — AFID Association")

    # Load registry
    try:
        reg_data = yaml.safe_load(open(registry_path))
    except Exception as e:
        err(f"Cannot read registry: {e}")
        sys.exit(1)

    registry: dict[str, str] = {}
    for entry in reg_data.get("afids", []) or []:
        if isinstance(entry, dict) and "id" in entry:
            registry[entry["id"]] = entry.get("description", "")

    info(f"Registry loaded — {len(registry)} AFID(s)")

    # Load map
    try:
        map_data = yaml.safe_load(open(afid_map_path))
    except Exception as e:
        err(f"Cannot read afid map: {e}")
        sys.exit(1)

    afid_map: dict[str, str] = {}
    for entry in map_data.get("mappings", []) or []:
        if isinstance(entry, dict) and "event" in entry and "afid" in entry:
            afid_map[entry["event"]] = entry["afid"]

    info(f"AFID map loaded  — {len(afid_map)} mapping(s)")

    # Associate
    matched = 0
    for event_id, ev in events.items():
        if event_id in afid_map:
            ev.afid = afid_map[event_id]
            matched += 1
            ok(f"{event_id} → {ev.afid} ({registry.get(ev.afid, 'unknown')})")
        else:
            warn(f"{event_id} → no AFID mapped")

    print()
    ok(f"{matched}/{len(events)} event(s) have AFID associations")
    return afid_map, registry


# ─────────────────────────────────────────────────────────────────────────────
# Pass 3 — Validation
# ─────────────────────────────────────────────────────────────────────────────

def pass3_validate(
    events: dict[str, EventInfo],
    afid_map: dict[str, str],
    registry: dict[str, str],
) -> tuple[bool, list[str], list[str]]:
    """
    Pass 3: Run all validation checks.

    Checks:
      V1 — Mapped event exists in PDI              (FAIL if not)
      V2 — Mapped AFID exists in registry          (FAIL if not)
      V3 — Every event has at least one primary param (FAIL if not)
      V4 — Primary param should be object_path type  (WARN if not)
      V5 — Events with AFID but no object_path param (WARN)
    """
    section("Pass 3 — Validation")

    errors:   list[str] = []
    warnings: list[str] = []

    # V1 + V2: mapped events and AFIDs
    info("V1/V2 — Checking mapped events exist in PDI and AFIDs exist in registry...")
    for event_id, afid in afid_map.items():
        if event_id not in events:
            msg = f"Mapped event '{event_id}' NOT found in PDI (stale mapping)"
            errors.append(msg)
            err(msg)
            # Mark it so it shows in CSV
            ghost = EventInfo(
                event_id=event_id, kind="", severity="",
                redfish_mapping="", afid=afid
            )
            ghost.valid = False
            ghost.issues.append("stale: not in PDI")
            events[event_id] = ghost
        else:
            ok(f"Event '{event_id}' — found in PDI")

        if afid not in registry:
            msg = f"AFID '{afid}' (for '{event_id}') NOT in registry"
            errors.append(msg)
            err(msg)
            if event_id in events:
                events[event_id].valid = False
                events[event_id].issues.append(f"invalid AFID: {afid}")
        else:
            ok(f"AFID '{afid}' — valid ({registry[afid]})")

    # V3 + V4 + V5: param checks on all PDI events that have an AFID
    print()
    info("V3/V4/V5 — Checking param requirements on mapped events...")
    for event_id, ev in events.items():
        if not ev.afid:
            continue  # skip unmapped events for param checks

        # V3: must have at least one primary param
        if not ev.has_primary:
            msg = f"Event '{event_id}' has no primary param"
            errors.append(msg)
            err(msg)
            ev.valid = False
            ev.issues.append("no primary param")
        else:
            ok(f"'{event_id}' — has primary param(s): "
               f"{[p.name for p in ev.primary_params]}")

        # V4: primary param should be object_path type
        for p in ev.primary_params:
            if p.type != "object_path":
                msg = (f"Event '{event_id}' — primary param '{p.name}' "
                       f"is type '{p.type}' not object_path")
                warnings.append(msg)
                warn(msg)
                ev.issues.append(
                    f"primary param '{p.name}' type is '{p.type}' not object_path")

        # V5: mapped event with no object_path param at all
        if not ev.has_object_path:
            msg = f"Event '{event_id}' has no object_path param"
            warnings.append(msg)
            warn(msg)
            ev.issues.append("no object_path param")

    passed = len(errors) == 0
    return passed, errors, warnings


# ─────────────────────────────────────────────────────────────────────────────
# CSV Output
# ─────────────────────────────────────────────────────────────────────────────

def write_csv(events: dict[str, EventInfo], registry: dict[str, str],
              out_path: str):
    """Write final report CSV — one row per event."""

    # Find max params across all events
    max_p = max((len(ev.params) for ev in events.values()), default=0)

    fields = [
        "event_id", "kind", "severity", "redfish_mapping",
        "afid", "afid_description",
        "has_primary", "has_object_path",
        "valid", "issues",
    ]
    for i in range(1, max_p + 1):
        fields += [f"param{i}_name", f"param{i}_type", f"param{i}_primary"]

    rows = []
    for ev in sorted(events.values(), key=lambda e: e.event_id):
        row = {
            "event_id":        ev.event_id,
            "kind":            ev.kind,
            "severity":        ev.severity,
            "redfish_mapping": ev.redfish_mapping,
            "afid":            ev.afid,
            "afid_description": registry.get(ev.afid, "") if ev.afid else "",
            "has_primary":     "true" if ev.has_primary else "false",
            "has_object_path": "true" if ev.has_object_path else "false",
            "valid":           "true" if ev.valid else "false",
            "issues":          " | ".join(ev.issues),
        }
        # Param columns
        for i in range(1, max_p + 1):
            if i <= len(ev.params):
                p = ev.params[i - 1]
                row[f"param{i}_name"]    = p.name
                row[f"param{i}_type"]    = p.type
                row[f"param{i}_primary"] = "true" if p.primary else "false"
            else:
                row[f"param{i}_name"]    = ""
                row[f"param{i}_type"]    = ""
                row[f"param{i}_primary"] = ""
        rows.append(row)

    with open(out_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)

    ok(f"CSV written → {Path(out_path).resolve()}")


# ─────────────────────────────────────────────────────────────────────────────
# Pass 4 — C++ Header Generation
# ─────────────────────────────────────────────────────────────────────────────

CPP_HEADER_TEMPLATE = """\
// GENERATED FILE — DO NOT EDIT
// Generated by: ael_pipeline.py
// Timestamp:    {timestamp}
//
// Source files:
//   AFID registry : {registry_path}
//   Event map     : {map_path}
//
// This header provides a zero-runtime-overhead AFID lookup table for use
// in the AEL (AMD Event Logging) enrichment layer (extensions::create()).

#pragma once

#include <string>
#include <unordered_map>

namespace amd::ael {{

// Event ID -> AFID mapping (build-time generated, only valid+mapped events)
static const std::unordered_map<std::string, std::string> eventToAfid = {{
{entries}
}};

/**
 * Look up the AFID for a given OpenBMC event ID.
 *
 * @param event  Fully-qualified event ID
 *               e.g. "xyz.openbmc_project.State.Fan.FanFailed"
 * @return       AFID string (e.g. "AFID_0x1001"), or "AFID_UNKNOWN" if not mapped
 */
inline const char* lookupAfid(const std::string& event)
{{
    auto it = eventToAfid.find(event);
    if (it != eventToAfid.end()) {{
        return it->second.c_str();
    }}
    return "AFID_UNKNOWN";
}}

}} // namespace amd::ael
"""


def pass4_generate_header(
    events: dict[str, "EventInfo"],
    registry: dict[str, str],
    afid_map_path: str,
    registry_path: str,
    header_path: str,
):
    """
    Pass 4: Generate the C++ header from validated+mapped events only.
    Skips any event that is invalid or has no AFID.
    """
    from datetime import datetime, timezone

    section("Pass 4 — C++ Header Generation")

    # Only include events that are valid AND have an AFID
    valid_mapped = {
        ev.event_id: ev.afid
        for ev in events.values()
        if ev.afid and ev.valid
    }

    if not valid_mapped:
        err("No valid mapped events — header will be empty. Check validation errors.")
        return

    # Sort for deterministic output
    sorted_entries = sorted(valid_mapped.items(), key=lambda x: x[0])

    entry_lines = []
    for event_id, afid in sorted_entries:
        desc = registry.get(afid, "")
        comment = f"  // {desc}" if desc else ""
        entry_lines.append(f'    {{"{event_id}", "{afid}"}},{comment}')

    entries_block = "\n".join(entry_lines)
    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    content = CPP_HEADER_TEMPLATE.format(
        timestamp     = timestamp,
        registry_path = str(Path(registry_path).resolve()),
        map_path      = str(Path(afid_map_path).resolve()),
        entries       = entries_block,
    )

    try:
        with open(header_path, "w") as f:
            f.write(content)
        ok(f"Header written → {Path(header_path).resolve()}")
        info(f"  Entries : {len(sorted_entries)} valid mapped event(s)")
        # Print the header so it's visible in terminal
        print()
        print(content)
    except IOError as e:
        err(f"Failed to write header: {e}")


# ─────────────────────────────────────────────────────────────────────────────
# Summary
# ─────────────────────────────────────────────────────────────────────────────

def print_summary(events, afid_map, registry, passed, errors, warnings,
                  out, header_path):
    section("Summary")

    total         = len(events)
    mapped        = sum(1 for ev in events.values() if ev.afid)
    unmapped      = total - mapped
    valid_mapped  = sum(1 for ev in events.values() if ev.afid and ev.valid)
    has_primary   = sum(1 for ev in events.values() if ev.has_primary)
    has_obj_path  = sum(1 for ev in events.values() if ev.has_object_path)

    print(f"  Total PDI events     : {bold(str(total))}")
    print(f"  Events with AFID     : {bold(str(mapped))}")
    print(f"  Events without AFID  : {yellow(str(unmapped)) if unmapped else green('0')}")
    print(f"  Valid mapped events  : {green(str(valid_mapped))}")
    print(f"  Events with primary  : {bold(str(has_primary))}")
    print(f"  Events with obj_path : {bold(str(has_obj_path))}")
    print(f"  Registry AFIDs       : {bold(str(len(registry)))}")
    print(f"  Errors               : {red(str(len(errors))) if errors else green('0')}")
    print(f"  Warnings             : {yellow(str(len(warnings))) if warnings else green('0')}")
    print(f"  Output CSV           : {cyan(str(Path(out).resolve()))}")
    if header_path:
        print(f"  Output header        : {cyan(str(Path(header_path).resolve()))}")
    print()

    if passed:
        print(f"  {green('✔  VALIDATION PASSED — header generated')}")
    else:
        print(f"  {red('✘  VALIDATION FAILED — header NOT generated')}")
        for e in errors:
            print(f"    {red('→')} {e}")
    print()


# ─────────────────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        prog="ael_pipeline.py",
        description="AEL multi-pass: discovery → association → validation → C++ header"
    )
    parser.add_argument("--pdi-dir",
        default="./phosphor-dbus-interfaces/yaml/xyz/openbmc_project")
    parser.add_argument("--amd-dir",
        default="",
        help="Optional AMD-local event YAML dir (e.g. yaml/com/amd/Event). "
             "Events here are discovered under the com.amd namespace.")
    parser.add_argument("--afid-map",
        default="./yaml/com/amd/ael/event_afid_map.yaml")
    parser.add_argument("--registry",
        default="./yaml/com/amd/ael/afid_registry.yaml")
    parser.add_argument("--out",
        default="./ael_report.csv",
        help="Output CSV report path")
    parser.add_argument("--header",
        default="./event_afid_map.hpp",
        help="Output C++ header path (default: ./event_afid_map.hpp)")
    parser.add_argument("--no-color", action="store_true")
    args = parser.parse_args()

    global _COLOR
    if args.no_color:
        _COLOR = False

    print()
    print(bold("  AEL Pipeline — Discovery → Association → Validation → C++ Header"))
    print()

    # ── Pass 1 ──────────────────────────────────────────────────────────────
    events = pass1_discover(args.pdi_dir, getattr(args, 'amd_dir', ''))

    # ── Pass 2 ──────────────────────────────────────────────────────────────
    afid_map, registry = pass2_associate(events, args.afid_map, args.registry)

    # ── Pass 3 ──────────────────────────────────────────────────────────────
    passed, errors, warnings = pass3_validate(events, afid_map, registry)

    # ── CSV ─────────────────────────────────────────────────────────────────
    section("Output — CSV")
    write_csv(events, registry, args.out)

    # ── Pass 4 — C++ Header (only if validation passed) ─────────────────────
    header_path = None
    if passed:
        header_path = args.header
        pass4_generate_header(
            events       = events,
            registry     = registry,
            afid_map_path = args.afid_map,
            registry_path = args.registry,
            header_path  = header_path,
        )
    else:
        section("Pass 4 — C++ Header Generation")
        err("Skipping header generation — validation failed. Fix errors above first.")

    # ── Summary ─────────────────────────────────────────────────────────────
    print_summary(events, afid_map, registry, passed, errors, warnings,
                  args.out, header_path)

    sys.exit(0 if passed else 1)


if __name__ == "__main__":
    main()