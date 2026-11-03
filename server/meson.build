server_sources = files(
  'server.c'
)

server_inc = include_directories(
  '.', 
  '..'
)

cc = meson.get_compiler('c')

enet6_dep = cc.find_library('enet6', required : true)

executable('server',
  server_sources,
  include_directories : server_inc,
  dependencies : [
    core_dep,
    dependency('iron', '>=2.0.0'),
    enet6_dep
  ],
  install : true
)
