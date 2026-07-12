# Mirrors the old CMakeLists.txt's file(GLOB "*.cpp") + list(REMOVE_ITEM
# ... ZonedChunkStorage.cpp) - see meson/list_sources.py for details.
world_sources_txt = run_command(
  python3_prog,
  list_sources_py,
  meson.current_source_dir(),
  'flat',
  '--ext', 'cpp',
  '--remove', 'ZonedChunkStorage.cpp',
  check: true,
).stdout().strip().split('\n')

world_sources = []
foreach rel : world_sources_txt
  world_sources += rel
endforeach

world_inc = include_directories(
  '.',
  'x64headers',
  '../Minecraft.Client/SDL3',
  '../Minecraft.Client/SDL3/stubs',
)

minecraft_world_lib = static_library(
  'Minecraft.World',
  world_sources,
  include_directories: world_inc,
  cpp_args: ['-include', meson.current_source_dir() / 'stdafx.h'],
  dependencies: [sdl3_dep, sdl3_image_dep, mimalloc_dep, zlib_ng_dep],
  install: false,
)

minecraft_world_dep = declare_dependency(
  link_with: minecraft_world_lib,
  include_directories: world_inc,
)
