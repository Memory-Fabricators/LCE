project(
  'lce',
  'cpp',
  version: '0.1.0',
  meson_version: '>=1.9.0',
  default_options: [
    'cpp_std=c++26',
    # 'b_lto=true',
    # 'b_lto_mode=thin',
    'buildtype=debugoptimized',
    'b_ndebug=if-release',
    'default_library=static',
  ],
)

if host_machine.system() == 'darwin'
  add_languages('objcpp', native: false)
endif

python_mod = import('python')
python3_prog = python_mod.find_installation('python3')
list_sources_py = files('meson/list_sources.py')

mimalloc_proj = subproject(
  'mimalloc',
  default_options: ['override=true', 'default_library=static'],
)
mimalloc_dep = mimalloc_proj.get_variable('mi_dep')
mimalloc_lib = mimalloc_proj.get_variable('mi_lib')

# Platform defines
common_cpp_args = ['-D_SDL3']
if get_option('debug')
  common_cpp_args += '-D_DEBUG'
else
  common_cpp_args += '-DNDEBUG'
endif

if get_option('lce_debug_menus')
  common_cpp_args += '-D_DEBUG_MENUS_ENABLED'
endif
if get_option('lce_large_worlds')
  common_cpp_args += '-D_LARGE_WORLDS'
endif
if get_option('lce_split_saves')
  common_cpp_args += '-DSPLIT_SAVES'
endif

add_project_arguments(common_cpp_args, language: 'cpp')
if host_machine.system() == 'darwin'
  add_project_arguments(common_cpp_args, language: 'objcpp')
endif

# Third-party dependencies
#
# Forced as subprojects (not plain dependency() calls) so these always
# build from the vendored wrap sources instead of silently picking up
# whatever the host system happens to have via pkg-config - which is what a
# plain dependency() call would otherwise prefer. Combined with
# default_library=static above, this is what makes the final
# Minecraft.Client binary fully statically linked against all of them.
# Resolution order matters here, because several of these subprojects call
# plain dependency('zlib')/dependency('libpng') internally, which meson
# auto-falls-back into building our zlib-ng.wrap/libpng.wrap subprojects the
# first time either name is resolved (matched via each wrap's [provide]
# section). If that first resolution happened implicitly inside another
# subproject() call below, our own explicit subproject('zlib-ng'/'libpng')
# calls would try to re-run those subprojects' meson.build a second time and
# fail on their own meson.override_dependency(...) calls ("already been
# resolved or overridden"). Resolving zlib-ng and libpng explicitly first -
# in that order, since libpng itself depends on zlib - means every other
# subproject's internal dependency('zlib'/'libpng') lookup just reuses these
# same already-overridden dependencies instead of triggering a second,
# conflicting execution.
zlib_ng_proj = subproject('zlib-ng', default_options: ['zlib-compat=true'])
zlib_ng_dep = zlib_ng_proj.get_variable('zlib_ng_dep')

libpng_proj = subproject('libpng')
libpng_dep = libpng_proj.get_variable('libpng_dep')

sdl3_proj = subproject('sdl3')
sdl3_dep = sdl3_proj.get_variable('sdl3_dep')

sdl3_image_proj = subproject('sdl3_image')
sdl3_image_dep = sdl3_image_proj.get_variable('sdl3_image_dep')

openal_proj = subproject('openal-soft')
openal_dep = openal_proj.get_variable('openal_dep')

tracy_proj = subproject('tracy', default_options: ['tracy_enable=true'])
tracy_dep = tracy_proj.get_variable('tracy_dep')

# GL/EGL remain plain system dependencies via pkg-config (per this
# project's Linux/Mesa integration).
gl_dep = dependency('gl')
egl_dep = dependency('egl')

subdir('Minecraft.World')
subdir('Minecraft.Client')
