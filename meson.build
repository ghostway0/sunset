project(
  'gaming',
  'c',
  default_options: ['c_std=gnu2x', 'warning_level=2', 'werror=true'],
)

add_project_arguments(
  [
    '-Wno-gnu-empty-struct',
    '-Wno-newline-eof',
    '-Wno-empty-translation-unit',
    '-Wno-gnu-statement-expression-from-macro-expansion',
    '-Wno-gnu-zero-variadic-macro-arguments',
    '-Wno-zero-length-array',
    '-Wimplicit-fallthrough',
    # '-D_GNU_SOURCE',
    '-g',
  ],
  language: 'c',
)

if target_machine.system() == 'linux'
  add_project_link_arguments(['-export-dynamic'], language: 'c')
endif

render_backend = get_option('render_backend')

if render_backend == 'opengl'
  add_project_arguments('-DSUNSET_BACKEND_OPENGL', language: 'c')
else
  error('unknown backend: ' + render_backend)
endif

is_debug = get_option('debug')
is_release = not is_debug
cc = meson.get_compiler('c')

cmocka_dep = dependency('cmocka')
glew_dep = dependency('glew', required: true)
glfw_dep = dependency('glfw3', required: true)
opengl_dep = dependency('opengl', required: true)

m_dep = cc.find_library('m', required: true)
c_dep = cc.find_library('c', required: true)

sunset_inc = include_directories('include/', 'include/sunset/')

subdir('external')

sunset_files = [
  'src/map.c',
  'src/btree.c',
  'src/crc64.c',
  'src/errors.c',
  'src/anim.c',
  'src/rman.c',
  'src/render.c',
  'src/fonts.c',
  'src/ring_buffer.c',
  'src/geometry.c',
  'src/camera.c',
  'src/input.c',
  'src/events.c',
  'src/json.c',
  'src/bitmask.c',
  'src/shader.c',
  'src/ecs.c',
  'src/base64.c',
  'src/backend.c',
  'src/octree.c',
  'src/ui.c',
  'src/engine.c',
  'src/commands.c',
  'src/tga.c',
  'src/filesystem.c',
  'src/byte_stream.c',
  'src/mtl_file.c',
  'src/io.c',
  'src/obj_file.c',
  'src/images.c',
  'src/utils.c',
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
    m_dep,
  ],
  include_directories: [sunset_inc],
)

sunset_dep = declare_dependency(
  link_with: sunset_lib,
  include_directories: [sunset_inc],
)

mods_lib = shared_library(
  'mods',
  files('mods/thing.c'),
  dependencies: [
    glew_dep,
    glfw_dep,
    opengl_dep,
    sunset_dep,
    logc_dep,
    cglm_dep,
    m_dep,
  ],
  include_directories: [sunset_inc],
)

world_builder = shared_library(
  'builder',
  files('world-builder/plugin.c'),
  dependencies: [
    glew_dep,
    glfw_dep,
    opengl_dep,
    sunset_dep,
    logc_dep,
    cglm_dep,
    m_dep,
  ],
  include_directories: [sunset_inc],
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
    m_dep,
    c_dep,
  ],
  include_directories: [sunset_inc],
)

tests_exe = executable(
  'tests',
  files('src/tests.c'),
  dependencies: [
    cmocka_dep,
    sunset_dep,
    cglm_dep,
    m_dep,
  ],
  include_directories: [sunset_inc],
)

test(
  'tests',
  tests_exe,
  suite: 'tests',
)

json_test_exe = executable(
  'json_test',
  files('src/json_test.c'),
  dependencies: [
    cmocka_dep,
    sunset_dep,
    m_dep,
  ],
  include_directories: [sunset_inc],
)

test(
  'json_test',
  json_test_exe,
  suite: 'tests',
)

obj_tests_exe = executable(
  'obj_tests',
  files('src/obj_test.c'),
  dependencies: [
    cmocka_dep,
    sunset_dep,
    cglm_dep,
    m_dep,
  ],
  include_directories: [sunset_inc],
)

test(
  'obj_tests',
  obj_tests_exe,
  suite: 'tests',
)

mtl_tests_exe = executable(
  'mtl_tests',
  files('src/obj_test.c'),
  dependencies: [
    cmocka_dep,
    sunset_dep,
    cglm_dep,
    m_dep,
  ],
  include_directories: [sunset_inc],
)

test(
  'mtl_tests',
  mtl_tests_exe,
  suite: 'tests',
)
