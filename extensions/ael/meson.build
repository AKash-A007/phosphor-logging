ael_sources = files(
    'entry_points.cpp',
    'manager.cpp',
)

ael_lib = static_library(
    'ael',
    ael_sources,
    include_directories: [
        include_directories('../..'),
        include_directories('../../gen'),
    ],
    dependencies: [
        sdbusplus_dep,
        sdeventplus_dep,
        phosphor_logging_dep,
        conf_h_dep,
    ],
)

ael_dep = declare_dependency(
    link_with: ael_lib,
)
