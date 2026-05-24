# Generated file; do not modify.
subdir('Oem')

sdbusplus_current_path = 'xyz/openbmc_project/Logging/Entry'

generated_markdown += custom_target(
    'xyz/openbmc_project/Logging/Entry/Oem__markdown'.underscorify(),
    input: [
        '../../../../../yaml/xyz/openbmc_project/Logging/Entry/Oem.interface.yaml',
    ],
    output: ['Oem.md'],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog,
        '--command',
        'markdown',
        '--output',
        meson.current_build_dir(),
        '--tool',
        sdbusplusplus_prog,
        '--directory',
        meson.current_source_dir() / '../../../../../yaml',
        'xyz/openbmc_project/Logging/Entry/Oem',
    ],
    install: should_generate_markdown,
    install_dir: [inst_markdown_dir / sdbusplus_current_path],
    build_by_default: should_generate_markdown,
)

