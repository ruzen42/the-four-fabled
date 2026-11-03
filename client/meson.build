client_sources = files(
  'game/colors.h',
  'game/game.h',
  'menu/menu.h',
  'net.c',
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

cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required : true)

raylib_dep = declare_dependency(
  dependencies : [
    cc.find_library('raylib', required : true)
  ],
  include_directories : include_directories() 
)

executable('client',
  client_sources,
  include_directories : client_inc,
  dependencies : [
    raylib_dep,
    m_dep,
    enet6_dep,
    core_dep,   
  ],
  install : true
)
