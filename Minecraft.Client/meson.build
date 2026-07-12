client_inc = include_directories(
  '.',
  'Common',
  'SDL3',
  'SDL3/stubs',
  'SDL3Media',
)

# Common GL-related defines shared by the main executable and the two
# per-file-standard helper libraries below (they all touch GL headers).
client_gl_args = [
  '-DGL_GLES_PROTOTYPES',
  '-DGL_GLEXT_PROTOTYPES',
  '-DTRACY_ENABLE',
  '-DLCE_USE_MESA_GL',
]

# ----------------------------------------------------------------------------
# Source enumeration (mirrors the old CMakeLists.txt file(GLOB...) /
# list(FILTER EXCLUDE REGEX...) / list(REMOVE_ITEM...) calls).

client_root_sources = run_command(
  python3_prog,
  list_sources_py,
  meson.current_source_dir(),
  'flat',
  '--ext', 'cpp',
  # Not part of the real project (absent from Minecraft.Client.vcxproj) -
  # dead code left over in the tree, calling APIs with stale signatures.
  '--remove', 'CreativeMode.cpp',
  check: true,
).stdout().strip().split('\n')

client_common_sources = run_command(
  python3_prog,
  list_sources_py,
  meson.current_source_dir(),
  'recursive',
  '--subdir', 'Common',
  '--ext', 'cpp',
  # TODO: Common/UI depends on the proprietary Iggy UI/font middleware, which
  # has no SDL3 equivalent yet. Excluded until UI rendering is reimplemented
  # for SDL3.
  '--exclude', '/Common/UI/',
  # Consoles_App.cpp unconditionally includes UI.h (Iggy) and Xbox-specific
  # headers with no platform guards - same Iggy dependency as Common/UI,
  # excluded alongside it.
  '--exclude', '/Common/Consoles_App\\.cpp',
  # SoundEngine.cpp is written entirely against the Miles Sound System (RAD
  # Game Tools), with no SDL3/OpenAL branch. Needs a real OpenAL-backed
  # reimplementation, same as the renderer - excluded until that's done.
  '--exclude', '/Common/Audio/SoundEngine\\.cpp',
  # Sony PSN-specific network code (sce toolkit, np.h) - only built for
  # PSVita/Orbis per Minecraft.Client.vcxproj, excluded on every other
  # platform including this one.
  '--exclude', '/Common/Network/Sony/',
  # Common/XUI is the Xbox360-era XUI framework (XUIMessageInit,
  # CXuiSceneImpl, etc.) - only built for Xbox 360 per the vcxproj, excluded
  # everywhere else including here.
  '--exclude', '/Common/XUI/',
  # Pulls in Common/UI/UIBitmapFont.h (Iggy) - same dependency gap as
  # Common/UI.
  '--exclude', '/Common/Tutorial/XuiCraftingTask\\.cpp',
  check: true,
).stdout().strip().split('\n')

# 4J_Render.cpp needs its own build settings (see below) and is compiled
# separately, so it is excluded from the generic SDL3/*.cpp source list.
client_sdl3_sources = run_command(
  python3_prog,
  list_sources_py,
  meson.current_source_dir(),
  'recursive',
  '--subdir', 'SDL3',
  '--ext', 'cpp',
  '--remove', 'SDL3/4JLibs/src/4J_Render.cpp',
  check: true,
).stdout().strip().split('\n')

client_sources = client_root_sources + client_common_sources + client_sdl3_sources + [
  # Platform-agnostic despite its name/location (a generic INetworkPlayer
  # wrapper around IQNetPlayer, no Xbox-specific dependencies) - reused by
  # PlatformNetworkManagerStub.cpp (the non-console networking stub) on
  # every platform without its own PlatformNetworkManager, SDL3 included.
  'Xbox/Network/NetworkPlayerXbox.cpp',
  # Just a hardcoded codepoint data table (SFontData) - no Iggy dependency,
  # unlike the rest of Common/UI, so it's fine to build despite the blanket
  # Common/UI/ exclusion above.
  'Common/UI/UIFontData.cpp',
]

client_deps = [
  minecraft_world_dep,
  sdl3_dep,
  sdl3_image_dep,
  libpng_dep,
  zlib_ng_dep,
  tracy_dep,
  gl_dep,
  egl_dep,
  mimalloc_dep,
]

client_link_args = []
client_extra_link_with = []

if host_machine.system() == 'darwin'
  quartzcore_dep = dependency('appleframeworks', modules: ['QuartzCore'])
  client_deps += quartzcore_dep
endif

lib_4j_render = static_library(
  'Minecraft.Client.4JRender',
  'SDL3/4JLibs/src/4J_Render.cpp',
  include_directories: client_inc,
  dependencies: client_deps,
  cpp_args: client_gl_args + ['-DLCE_SKIP_STDAFX'],
  override_options: ['cpp_std=c++26'],
  install: false,
)
client_extra_link_with += lib_4j_render

if host_machine.system() == 'darwin'
  # See the 4J_Render.cpp comment above - same "separate target" reasoning,
  # but this one skips stdafx.h completely rather than just the World/Common
  # types (LCE_SKIP_STDAFX_ENTIRELY): it only needs Apple's own BOOL/Component
  # types from QuartzCore/CoreServices.
  lib_4j_render_metal = static_library(
    'Minecraft.Client.4JRenderMetal',
    'SDL3/4JLibs/src/4J_RenderMetal.mm',
    include_directories: client_inc,
    dependencies: client_deps,
    objcpp_args: client_gl_args + ['-DLCE_SKIP_STDAFX_ENTIRELY'],
    install: false,
  )
  client_extra_link_with += lib_4j_render_metal
endif

# Assets are loaded via CWD-relative paths but live in the source tree, not
# next to the built binary. Bake in the source dir as the default assets
# root so the game runs from any working directory (overridable at runtime
# with the LCE_ASSETS_DIR env var - see main() in SDL3_Minecraft.cpp).
client_compile_args = client_gl_args + [
  '-DLCE_ASSETS_DIR_DEFAULT="@0@"'.format(meson.current_source_dir()),
  # Force stdafx.h ahead of every other header in every TU of this target -
  # see the "Precompiled headers disabled" comment on cpp_pch below for why
  # this replaces cpp_pch rather than just being removed outright.
  '-include', meson.current_source_dir() / 'stdafx.h',
]

minecraft_client_exe = executable(
  'Minecraft.Client',
  client_sources,
  include_directories: client_inc,
  # Precompiled headers disabled: Meson/Ninja's PCH staleness tracking here
  # doesn't reliably follow transitive header changes (edits to
  # Minecraft.World/*.h, which stdafx.h pulls in indirectly through half the
  # tree, would leave a stale .pch that Clang then refuses to reuse with a
  # hard "file has been modified since the precompiled header was built"
  # error - and a plain `ninja` re-run doesn't regenerate it, only `meson
  # setup --reconfigure`/a clean rebuild does). Not worth the intermittent
  # broken incremental builds for a legacy codebase this size.
  #
  # stdafx.h is instead force-included via -include below (added to
  # client_compile_args), not left to the plain #include "stdafx.h" line
  # every .cpp already has: throughout this codebase a .cpp's *own* header
  # is #include'd before stdafx.h textually, and those headers rely on
  # macros stdafx.h defines (AUTO_VAR, etc.) - something that only worked
  # before because the PCH forcibly injected stdafx.h's content ahead of the
  # whole TU regardless of #include order. -include reproduces that same
  # ordering guarantee without needing an actual precompiled/cached
  # artifact.
  # cpp_pch: 'pch/stdafx_pch.h',
  dependencies: client_deps,
  link_with: client_extra_link_with,
  # link_whole (not link_with/dependencies) is what actually makes mimalloc
  # the process-wide default allocator: it forces every object file from
  # mimalloc_lib - including alloc-override.o's aliased malloc/free/operator
  # new/delete symbols - into the final binary, rather than letting the
  # linker silently drop them as "unreferenced" (which is exactly what a
  # plain static-lib link normally does, since nothing in this project calls
  # mi_malloc()/mi_free() etc. directly). See the comment above mimalloc_proj
  # in the top-level meson.build for the full rationale.
  link_whole: mimalloc_lib,
  cpp_args: client_compile_args,
  win_subsystem: 'windows',
  install: true,
)