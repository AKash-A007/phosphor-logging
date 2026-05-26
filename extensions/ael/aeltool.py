#!/usr/bin/env python3
import json
import argparse
import subprocess
import sys
import os

REGISTRY_PATH = "/usr/share/phosphor-logging/ael/ael-registry.json"
if not os.path.exists(REGISTRY_PATH):
    # Fallback for local development
    REGISTRY_PATH = "./ael-registry.json"

INTERFACE_OEM = "xyz.openbmc_project.Logging.Entry.Oem"

def load_registry():
    try:
        with open(REGISTRY_PATH, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading registry: {e}")
        return {}

def run_command(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        return None
    return result.stdout.strip()

def list_events():
    """List all log entries that have the AEL OEM interface."""
    print(f"{'Path':<50} {'AFID':<10} {'Description'}")
    print("-" * 100)
    
    # Find all objects with the OEM interface
    objs = run_command(f"busctl tree xyz.openbmc_project.Logging --list | grep {INTERFACE_OEM}")
    if not objs:
        # Try a different approach if tree --list fails
        objs = run_command("busctl introspect xyz.openbmc_project.Logging / --recursive | grep -B 20 " + INTERFACE_OEM + " | grep '^node' | awk '{print $2}'")

    if not objs:
        print("No AEL events found.")
        return

    registry = load_registry().get("mappings", {})

    for path in objs.splitlines():
        # Get OemAdditionalData property
        props = run_command(f"busctl get-property xyz.openbmc_project.Logging {path} {INTERFACE_OEM} OemAdditionalData")
        if not props:
            continue
            
        # Parse output like 'a{ss} 2 "AEL.AFID" "0x1001" "AEL.VERSION" "1.0"'
        afid = "Unknown"
        if "AEL.AFID" in props:
            parts = props.split('"')
            for i, part in enumerate(parts):
                if part == "AEL.AFID":
                    afid = parts[i+2]
                    break
        
        # Cross-reference with registry for description
        desc = "No description available"
        for _, val in registry.items():
            if val.get("afid") == afid:
                desc = val.get("description", desc)
                break
                
        print(f"{path:<50} {afid:<10} {desc}")

def inject_event(event_id):
    """Inject a test event using the Logging.Create method."""
    registry = load_registry().get("mappings", {})
    if event_id not in registry:
        print(f"Error: Event '{event_id}' not found in registry.")
        return

    print(f"Injecting event: {event_id}")
    
    # Call Logging.Create(Message, Severity, AdditionalData)
    # Note: This tool is simple, so it uses default severity.
    cmd = f'busctl call xyz.openbmc_project.Logging /xyz/openbmc_project/logging xyz.openbmc_project.Logging.Create Create ssa{{ss}} "{event_id}" "xyz.openbmc_project.Logging.Entry.Level.Informational" 0'
    
    result = run_command(cmd)
    if result:
        print(f"Event created. Result: {result}")
    else:
        print("Failed to create event.")

def main():
    parser = argparse.ArgumentParser(description="AEL Tool - Manage AMD Event Layer logs")
    subparsers = parser.add_subparsers(dest="command")

    # List command
    subparsers.add_parser("list", help="List all AEL-decorated log entries")

    # Inject command
    inject_parser = subparsers.add_parser("inject", help="Inject a test event")
    inject_parser.add_argument("event", help="OpenBMC event ID (e.g., xyz.openbmc_project.State.Fan.FanFailed)")

    args = parser.parse_args()

    if args.command == "list":
        list_events()
    elif args.command == "inject":
        inject_event(args.event)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
