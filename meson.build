project(
    'bigint_c',
    'c',
    license: 'MIT',
    meson_version: '>=1.3.0',
    default_options: {
        'buildtype': 'release',
        'optimization': '3',
        'c_std': ['c2y', 'c23', 'c2x', 'c11'],
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

args = ['-DBIGINT_C_VERSION_STRING="' + bigint_c_lib_version + '"', '-DBIGINT_C_LIB_TYPE='
            + (get_option('default_library') == 'static' ? '1' : '0')]

private_args = []

underlying_computation = get_option('underlying_computation')

cc = meson.get_compiler('c')

if underlying_computation == 'auto'

    support_128_bits = cc.compiles(
        '''
#if defined __GNUC__ && defined(__SIZEOF_INT128__)
#else
#error "128 bit not supported"
#endif

int main(void){return 0;}
'''
    )

    if support_128_bits
        underlying_computation = '128bit'
    else
        underlying_computation = 'normal'
    endif

elif underlying_computation == 'normal'
    # do nothing, this always works
elif underlying_computation == '128bit'
    # check if we support that, otherwise fall back to normal

    support_128_bits = cc.compiles(
        '''
#if defined __GNUC__ && defined(__SIZEOF_INT128__)
#else
#error "128 bit not supported"
#endif

int main(void){return 0;}
'''
    )

    if support_128_bits
        underlying_computation = '128bit'
    else
        warning('User wanted to use "128bit" but that is not supported, using "normal" fallback')
        underlying_computation = 'normal'
    endif
else
    error('Unsupported underlying_computation: ' + underlying_computation)
endif

message('Using underlying computation: ' + underlying_computation)
if underlying_computation == 'normal'
    private_args += ('-DBIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION=1')
elif underlying_computation == '128bit'
    private_args += ('-DBIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION=0')
else
    error('Unsupported underlying_computation: ' + underlying_computation)
endif


bigint_c_lib = library(
    'bigint_c',
    src_files,
    dependencies: deps,
    include_directories: inc_dirs,
    c_args: args + private_args,
    c_shared_args: ['-DBIGINT_C_LIB_EXPORT'],
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
    extra_cflags: args,
)

bigint_c_dep = declare_dependency(
    link_with: bigint_c_lib,
    compile_args: args,
    include_directories: inc_dirs,
    dependencies: deps,
    version: bigint_c_lib_version,
)

meson.override_dependency('bigint_c', bigint_c_dep)

tests = get_option('tests')

if tests != 'false'
    subdir('test')
endif
