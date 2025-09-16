test_deps = deps

test_deps += dependency('gtest', version:'>=1.17.0')

test_deps += bigint_c_dep

test_src = files('entry.cpp')

test_files = [
    'initialization_c.cpp',
]

add_languages('cpp',required:true, native:false)

foreach file : test_files
    file_name = file.split('.')[-2]
    test_exe = executable(
        'test_' + file_name,
        test_src,
        files(file),
        dependencies: test_deps,
        override_options: {
            'warning_level': '3',
            'werror': true,
            'b_coverage': false,
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
