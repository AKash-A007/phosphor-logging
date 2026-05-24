# extensions/ael/meson.build

python3_prog = find_program('python3')

# ── Step 1: Generate event_afid_map.hpp at build time ────────────────────────

event_afid_map_hpp = custom_target(
    'ael_afid_codegen',
    command: [
        python3_prog,
        files('../../tools/ael_pipeline.py'),
        '--pdi-dir',  meson.project_source_root() / 'subprojects/phosphor-dbus-interfaces/yaml/xyz/openbmc_project',
        '--amd-dir',  meson.project_source_root() / 'yaml/com/amd',
        '--afid-map', meson.project_source_root() / 'yaml/com/amd/ael/event_afid_map.yaml',
        '--registry', meson.project_source_root() / 'yaml/com/amd/ael/afid_registry.yaml',
        '--out',      meson.current_build_dir() / 'ael_build_report.csv',
        '--header',   '@OUTPUT@',
        '--no-color',
    ],
    output: 'event_afid_map.hpp',
    build_by_default: true,
)

# ── Step 2: AEL sources ───────────────────────────────────────────────────────
# entry_points.cpp  — startup + REGISTER_EXTENSION_FUNCTION (high-level entry)
# manager.cpp       — Manager class (owns all state)
# ael_entry.cpp     — per-entry OEM object (owns D-Bus interface)

ael_sources = files(
    'entry_points.cpp',
    'manager.cpp',
    'ael_entry.cpp',
)

# ── Step 3: Dependencies ──────────────────────────────────────────────────────

ael_deps = [
    sdbusplus_dep,
    phosphor_logging_dep,
    dependency('phosphor-dbus-interfaces'),
]

ael_include_dirs = include_directories(
    '.',
)

# ── Step 5: Tests ─────────────────────────────────────────────────────────────

# Note: Tests are handled by test/meson.build which calls subdir('ael')

# ── Step 6: Expose to root meson.build ────────────────────────────────────────
#
# In phosphor-logging/meson.build:
#
#   if get_option('ael-extension').allowed()
#       subdir('extensions/ael')
#       phosphor_log_manager_sources  += ael_sources
#       phosphor_log_manager_deps     += ael_deps
#       phosphor_log_manager_deps     += [event_afid_map_hpp]
#       phosphor_log_manager_includes += [ael_include_dirs]
#   endif