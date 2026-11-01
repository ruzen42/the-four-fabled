client_sources = files(
  'game/colors.h',
  'game/game.h',
  'menu/menu.h',
  'client.c',
  'game/game.c',
  'menu/menu.c'
)

client_inc = include_directories(
  '.', 
  'game', 
  'menu',
  '..'
)

raylib_dep = dependency('raylib', required : false)

executable('client',
  client_sources,
  include_directories : client_inc,
  dependencies : [
    raylib_dep,
    core_dep,   
  ],
  install : true
)
