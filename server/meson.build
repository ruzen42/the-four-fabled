server_sources = files(
  'server.c'
)

client_inc = include_directories(
  '.', 
  '..'
)

executable('server',
  server_sources,
  dependencies : [
    core_dep
  ],
  install : true
)
