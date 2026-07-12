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
  # PCH disabled (see the comment above cpp_pch in Minecraft.Client's own
  # meson.build for the stale-PCH ninja rebuild failures that motivated
  # this), but stdafx.h must still be force-included ahead of every other
  # header in every TU via -include, not left to a plain #include "stdafx.h"
  # line: throughout this codebase (Tag.cpp -> ListTag.h being one example)
  # a .cpp's *own* header is #include'd before stdafx.h textually, and those
  # headers rely on macros stdafx.h defines (AUTO_VAR, etc.) - something
  # that only worked before because the PCH forcibly injected stdafx.h's
  # content ahead of the whole TU regardless of #include order. -include
  # reproduces exactly that ordering guarantee without needing an actual
  # precompiled/cached artifact.
  # cpp_pch: 'pch/stdafx_pch.h',
  cpp_args: ['-include', meson.current_source_dir() / 'stdafx.h'],
  # mimalloc_dep here is only for its include_directories (mi_malloc() etc.
  # are used directly nowhere in this static lib); the actual process-wide
  # malloc/free/new/delete override happens once, at final-executable link
  # time, by link_whole-ing mimalloc_lib into Minecraft.Client - see the
  # comment above mimalloc_proj in the top-level meson.build.
  dependencies: [sdl3_dep, mimalloc_dep],
  install: false,
)

minecraft_world_dep = declare_dependency(
  link_with: minecraft_world_lib,
  include_directories: world_inc,
)
