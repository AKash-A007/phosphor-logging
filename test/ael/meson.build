# test/ael/meson.build

gtest_dep = dependency('gtest', main: true)

test(
    'test-ael',
    executable(
        'test-ael',
        'test_ael.cpp',
        dependencies: [
            gtest_dep,
            sdbusplus_dep,
            phosphor_logging_dep,
            event_afid_map_hpp,     # generated header as dep
        ],
        include_directories: [
            include_directories('../..'),
            ael_include_dirs,
        ],
        link_with: [],
    ),
    timeout: 60,
)