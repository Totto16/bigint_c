test_deps = deps

test_deps += dependency(
    'gtest',
    version: '>=1.17.0',
)
test_deps += dependency(
    'gmock',
    version: '>=1.17.0',
)

if host_machine.system() == 'windows'
    test_deps += dependency('libtommath')
else
    test_deps += dependency('gmp')
endif


test_deps += bigint_c_dep

test_src = []

subdir('helper')
subdir('src')

test_files = ['normal.cpp', 'raw_c_tests.cpp', 'cpp_only_tests.cpp', 'compile_time_tests.cpp']

add_languages(
    'cpp',
    required: true,
    native: false,
)

foreach file : test_files
    file_name = file.split('.')[-2]

    test_args = []

    if file_name == 'compile_time_tests'

        cxx = meson.get_compiler('cpp')

        test_needed_constexpr_depth = 1024
        test_needed_constexpr_steps = 1000000000

        if cxx.get_id() == 'gcc'
            #default 512
            test_args += ('-fconstexpr-depth=' + test_needed_constexpr_depth.to_string())
            # default: 33554432 (1<<25)
            test_args += (
                '-fconstexpr-ops-limit='
                                        + test_needed_constexpr_steps.to_string()
            )
        elif cxx.get_id() == 'clang'
            test_args += ('-fconstexpr-depth=' + test_needed_constexpr_depth.to_string())
            test_args += ('-fconstexpr-steps=' + test_needed_constexpr_steps.to_string())
        elif host_machine.system() == 'windows'
        
        else
            warning('Not supported test \''
                + file_name + '\' for compiler \'' + cxx.get_id() + '\'')
            continue
        endif

    endif

    test_exe = executable(
        'test_' + file_name,
        test_src,
        files('tests' / file),
        dependencies: test_deps,
        cpp_args: test_args,
        override_options: {
            'warning_level': '3',
            'werror': true,
            'cpp_std': ['c++23', 'c++latest', 'c++20'],
        },
    )

    test(
        'test_' + file_name,
        test_exe,
        protocol: 'gtest',
        is_parallel: true,
    )

endforeach
