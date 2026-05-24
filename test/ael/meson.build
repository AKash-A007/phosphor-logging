# test/ael/meson.build

# test/ael/meson.build
# gtest_dep is inherited from parent test/meson.build (googletest subproject)

test(
    'test-ael',
    executable(
        'test-ael',
        'test_ael.cpp',
        dependencies: [
            conf_h_dep,
            gtest_dep,
            gmock_dep,
            sdbusplus_dep,
            sdeventplus_dep,
            pdi_dep,
            phosphor_logging_dep,
            declare_dependency(sources: [event_afid_map_hpp]),
        ],
        include_directories: [
            include_directories('../..'),
            include_directories('../../gen'),
            ael_include_dirs,
        ],
        link_with: [log_manager_lib],
    ),
    timeout: 60,
)