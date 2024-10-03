project('gaming', 'c', default_options: ['c_std=c2x', 'warning_level=3', 'werror=true'])

is_debug = get_option('debug')
is_release = not is_debug
cc = meson.get_compiler('c')

add_global_arguments(['-march=native'], language: 'c')

cmocka_dep = dependency('cmocka')
glew_dep = dependency('glew', required: true)
glfw_dep = dependency('glfw3', required: true)
opengl_dep = dependency('opengl', required: true)

gaming_inc = include_directories('include/')

subdir('external')

sunset_lib = static_library(
  'sunset',
  files('src/color.c', 'src/ring_buffer.c'),
  include_directories: [gaming_inc],
  dependencies: [
    glew_dep,
    glfw_dep,
    opengl_dep,
  ],
)

sunset_dep = declare_dependency(
  link_with: sunset_lib,
)

executable(
  'gaming',
  files('src/main.c'),
  dependencies: [
    logc_dep,
    glew_dep,
    glfw_dep,
    opengl_dep,
    sunset_dep,
  ],
  include_directories: [gaming_inc],
)

tests_exe = executable(
  'tests',
  files('src/tests.c'),
  dependencies: [
    cmocka_dep,
    logc_dep,
    sunset_dep,
  ],
  include_directories: [gaming_inc],
)
