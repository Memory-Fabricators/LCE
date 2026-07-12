project('sdl3_image', 'c',
  version: '3.2.4')

sdl_dep = dependency('sdl3', version: '>=3.2.4')

png_dep = dependency('libpng')
jpg_dep = dependency('libjpeg', required: false)

# Fixed vs. the upstream wrapdb sdl3_image_3.2.4-1 patch: that overlay never
# defines any of the LOAD_* macros (LOAD_PNG, LOAD_JPG, etc.) that every
# format loader in src/IMG_*.c is #ifdef-gated behind - see e.g. IMG_png.c's
# "#ifdef LOAD_PNG" wrapped around its entire libpng-backed implementation.
# SDL_image's own CMakeLists.txt sets these per format as it detects each
# optional dependency (PNG::PNG, libjpeg, etc.); the meson build has no
# equivalent step at all, so IMG_Load()/IMG_isPNG() et al. silently compile
# down to their format-table entries being present but every actual is()/
# load() call unreachable - not a link or compile error, just
# "Unsupported image format" at runtime for every file. This project's
# assets are exclusively PNG (see Minecraft.Client/Common), so only that
# loader is enabled - see png_dep above for the actual libpng dependency
# this needs.
img_args = ['-DLOAD_PNG']
subdir('src')
img_lib = library('sdl3_image',
  img_src,
  c_args: img_args,
  include_directories: 'include',
  dependencies: [png_dep, sdl_dep])

sdl3_image_dep = declare_dependency(include_directories: 'include',
  link_with: img_lib)
