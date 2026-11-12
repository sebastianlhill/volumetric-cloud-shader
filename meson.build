project('volumetric_clouds', ['c', 'cpp'], version: '0.1', 
  default_options : ['warning_level=3','c_std=c11', 'c_std=c11'])

glfw_dep = dependency('glfw3', required : true)
gl_dep   = dependency('gl', required : true)

inc = include_directories('include')

executable('volumetric_clouds', 
  sources: [
    'src/main.cpp', 
    'src/shader.cpp',
    'src/cloudbake.cpp',
    'src/glad.c',
    'src/TileableVolumeNoise.cpp',
    'src/volumeio.cpp',
    ],
  include_directories : inc,
  dependencies : [glfw_dep, gl_dep],
  install : true)
