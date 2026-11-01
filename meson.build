project(
  'four-fabled',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : ['warning_level=3'],
)

dependencies = [
  dependency('raylib', version : '>=4.5.0')
]

sources = [
  'core/world.c',
  'game/game.c',
  'client.c',
]

exe = executable(
  'game',
  sources,
  dependencies : dependencies,
  install : true,
)

