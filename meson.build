project('the_four_fabled', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3', 
    'c_std=c11',
    'default_library=static'
  ],
)


cc = meson.get_compiler('c')
enet6_dep = cc.find_library('enet6', required : true)

subdir('core')

subdir('client')
subdir('server')
