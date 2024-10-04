project(
  'gaming',
  'c',
  default_options: ['c_std=gnu2x', 'warning_level=3', 'werror=true'],
)

add_project_arguments(['-Wno-gnu-empty-struct', '-Wno-newline-eof', '-Wno-empty-translation-unit'], language: 'c')

is_debug = get_option('debug')
is_release = not is_debug
cc = meson.get_compiler('c')

cmocka_dep = dependency('cmocka')
glew_dep = dependency('glew', required: true)
glfw_dep = dependency('glfw3', required: true)
opengl_dep = dependency('opengl', required: true)

sunset_inc = include_directories('include/')

subdir('external')

sunset_files = [
  'src/color.c',
  'src/errors.c',
  'src/fonts.c',
  'src/ring_buffer.c',
  'src/gfx.c',
  'src/camera.c',
  'src/input.c',
  'src/scene.c',
]

sunset_lib = static_library(
  'sunset',
  files(sunset_files),
  dependencies: [
    glew_dep,
    glfw_dep,
    opengl_dep,
    logc_dep,
    cglm_dep,
  ],
  include_directories: [sunset_inc],
)

sunset_dep = declare_dependency(
  link_with: sunset_lib,
)

executable(
  'sunset',
  files('src/main.c'),
  dependencies: [
    logc_dep,
    glew_dep,
    glfw_dep,
    opengl_dep,
    sunset_dep,
    cglm_dep,
  ],
  include_directories: [sunset_inc],
)

tests_exe = executable(
  'tests',
  files('src/tests.c'),
  dependencies: [
    cmocka_dep,
    logc_dep,
    sunset_dep,
  ],
  include_directories: [sunset_inc],
)
