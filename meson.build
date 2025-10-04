project(
    'bigint_c',
    'c',
    license: 'MIT',
    meson_version: '>=1.3.0',
    default_options: {
        'buildtype': 'release',
        'optimization': '3',
        'c_std': ['c2y', 'c23', 'c2x','c11'],
        'warning_level': '3',
        'werror': 'true',
        'b_ndebug': 'if-release',
    },
    version: '0.0.1',
)

deps = []

cc = meson.get_compiler('c')

deps += cc.find_library(
    'm',
    required: false,
)


src_files = []
inc_dirs = []

bigint_c_lib_version = meson.project_version()

bigint_c_lib_header_install_dir = 'bigint_c'

subdir('src')


bigint_c_lib = library(
    'bigint_c',
    src_files,
    dependencies: deps,
    include_directories: inc_dirs,
    c_args: ['-DBIGINT_C_VERSION_STRING="' + bigint_c_lib_version + '"'],
    install: true,
    version: bigint_c_lib_version,
)

pkg = import('pkgconfig')

pkg.generate(
    bigint_c_lib,
    description: 'A bigint library in c',
    name: 'bigint_c',
    filebase: 'bigint_c',
    subdirs: [bigint_c_lib_header_install_dir / 'public'],
)

bigint_c_dep = declare_dependency(
    link_with: bigint_c_lib,
    include_directories: inc_dirs,
    dependencies: deps,
    version: bigint_c_lib_version,
)

meson.override_dependency('bigint_c', bigint_c_dep)

if get_option('tests')
    subdir('test')
endif
