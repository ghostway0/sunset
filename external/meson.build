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
