project('volume', ['c', 'cpp'], version: '0.1', 
  default_options : ['warning_level=3','c_std=c11', 'cpp_std=c++20'])

glfw_dep = dependency('glfw3', required : true)
gl_dep   = dependency('gl', required : true)

inc = include_directories('include')

executable('volume', 
  sources: [
    'src/main.cpp', 
    'src/volumeio.cpp',
    'src/cloudbake.cpp',
    'src/TileableVolumeNoise.cpp'
    ],
  include_directories : inc,
  dependencies : [glfw_dep, gl_dep],
  install : true)
