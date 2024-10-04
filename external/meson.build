logc_lib = static_library(
  'logc',
  'log.c/src/log.c',
  include_directories: include_directories('log.c/src/'),
  c_args: ['-DLOG_USE_COLOR', '-std=c99'],
)

logc_dep = declare_dependency(
  include_directories: include_directories('log.c/src/'),
  link_with: logc_lib,
)

cglm_files = [
  'cglm/src/mat2x3.c',
  'cglm/src/quat.c',
  'cglm/src/ease.c',
  'cglm/src/vec4.c',
  'cglm/src/affine.c',
  'cglm/src/mat2.c',
  'cglm/src/curve.c',
  'cglm/src/clipspace/persp_lh_no.c',
  'cglm/src/clipspace/ortho_lh_no.c',
  'cglm/src/clipspace/view_rh_no.c',
  'cglm/src/clipspace/project_no.c',
  'cglm/src/clipspace/view_lh_no.c',
  'cglm/src/clipspace/ortho_rh_no.c',
  'cglm/src/clipspace/persp_rh_no.c',
  'cglm/src/clipspace/view_rh_zo.c',
  'cglm/src/clipspace/persp_lh_zo.c',
  'cglm/src/clipspace/ortho_lh_zo.c',
  'cglm/src/clipspace/project_zo.c',
  'cglm/src/clipspace/ortho_rh_zo.c',
  'cglm/src/clipspace/persp_rh_zo.c',
  'cglm/src/clipspace/view_lh_zo.c',
  'cglm/src/mat4x3.c',
  'cglm/src/ivec3.c',
  'cglm/src/ray.c',
  'cglm/src/io.c',
  'cglm/src/mat2x4.c',
  'cglm/src/affine2d.c',
  'cglm/src/mat3x4.c',
  'cglm/src/vec3.c',
  'cglm/src/project.c',
  'cglm/src/euler.c',
  'cglm/src/ivec4.c',
  'cglm/src/mat4.c',
  'cglm/src/box.c',
  'cglm/src/aabb2d.c',
  'cglm/src/vec2.c',
  'cglm/src/swift/empty.c',
  'cglm/src/cam.c',
  'cglm/src/bezier.c',
  'cglm/src/mat3x2.c',
  'cglm/src/sphere.c',
  'cglm/src/frustum.c',
  'cglm/src/mat3.c',
  'cglm/src/ivec2.c',
  'cglm/src/mat4x2.c',
  'cglm/src/plane.c',
]

cglm_lib = static_library(
  'cglm',
  files(cglm_files),
  include_directories: include_directories('cglm/include/'),
  c_args: ['-std=c99'],
)

cglm_dep = declare_dependency(
  include_directories: include_directories('cglm/include/'),
  link_with: cglm_lib,
)
