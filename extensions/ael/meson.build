ael_sources = files(
    'entry_points.cpp',
    'manager.cpp',
    'serializer.cpp',
    'validator.cpp',
)

ael_include_dirs = include_directories('.')

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
        phosphor_logging_dep,
        nlohmann_json_dep,
        conf_h_dep,
    ],
)

ael_dep = declare_dependency(
    link_with: ael_lib,
    include_directories: ael_include_dirs,
)

# Install AEL Registry
install_data(
    'ael-registry.json',
    install_dir: get_option('datadir') / 'phosphor-logging/ael',
)
