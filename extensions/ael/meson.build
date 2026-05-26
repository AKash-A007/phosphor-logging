# extensions/ael/meson.build

ael_sources = files(
    'ael_entry.cpp',
    'entry_points.cpp',
    'manager.cpp',
)

ael_include_dirs = include_directories('.')

# The actual extension logic will be added in Commit 2
# For Commit 1, we just provide the basic infrastructure
ael_lib = static_library(
    'ael',
    ael_sources,
    include_directories: [
        include_directories('../..'),
        include_directories('../../gen'),
        ael_include_dirs,
    ],
    dependencies: [
        sdbusplus_dep,
        sdeventplus_dep,
        pdi_dep,
        phosphor_logging_dep,
        conf_h_dep,
    ],
)

ael_dep = declare_dependency(
    link_with: ael_lib,
    include_directories: ael_include_dirs,
)
