core_sources = files(
  'types.h',
  'net/net.h',
  'net/protocol/protocol.h',
  'world.h',
  'world.c',
  'game.h',
  'game.c',
  'lobby.h',
  'lobby.c',
  'net/net.c'
)

core_inc = include_directories('.')

core_lib = static_library('core',
  core_sources,
  include_directories : core_inc
)

core_dep = declare_dependency(
  link_with : core_lib,
  include_directories : core_inc
)
