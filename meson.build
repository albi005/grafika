project('grafika_sablon', ['cpp', 'c'], default_options: ['cpp_std=c++17', 'warning_level=3', 'werror=false'])

# Compiler flags
add_project_arguments('-Wall', '-Wextra', '-pedantic', '-D_HAS_CXX17=1', language: 'cpp')

# Dependencies
deps = [
    dependency('gl', required: true),
    dependency('glew', required: true),
    dependency('glfw3', required: true),
    dependency('glm', required: true),
    dependency('glu', required: true),
    dependency('glut', required: true),
]

# External Libraries
libs = [
    static_library('glad', 'external/glad/src/glad.c', include_directories: 'external/glad/include'),
    static_library('lodepng', 'external/lodepng/lodepng.cpp', include_directories: 'external/lodepng')
]

include_directories = [
    'external/glad/include',
    'external/lodepng'
]

targets = [
    'green',
    'hf1',
]

foreach target : targets
    executable(
        target,
        files(
            'src/' + target + '.cpp',
            'framework.h',
            'framework.cpp'
        ),
        dependencies: deps,
        link_with: libs,
        include_directories: include_directories,
    )
endforeach
