server_sources = files(
  'server.c'
)

server_inc = include_directories(
  '.', 
  '..'
)

executable('server',
  server_sources,
  include_directories : server_inc,
  dependencies : [
    core_dep,
    dependency('iron', version : '>=2.0.0'),
    enet6_dep
  ],
  install : true
)
