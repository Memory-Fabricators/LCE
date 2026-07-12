# Fixed vs. the upstream wrapdb sdl3_image_3.2.4-1 patch: that overlay's
# img_src list omits IMG.c entirely, which defines the library's actual
# public entry points (IMG_Load, IMG_Load_IO, IMG_LoadTyped_IO, IMG_Init,
# etc. - see include/SDL3_image/SDL_image.h) that every one of the
# format-specific IMG_*.c loaders below is dispatched from. Without it the
# library builds and installs "successfully" but leaves every caller of
# IMG_Load()/IMG_Load_IO() (e.g. Minecraft.Client/SDL3/4JLibs/src/4J_Render.cpp)
# with an undefined-symbol link error.
img_src = files(
  'IMG.c',
  'IMG_avif.c',
  'IMG_bmp.c',
  'IMG_gif.c',
  'IMG_jpg.c',
  'IMG_jxl.c',
  'IMG_lbm.c',
  'IMG_pcx.c',
  'IMG_png.c',
  'IMG_pnm.c',
  'IMG_qoi.c',
  'IMG_stb.c',
  'IMG_svg.c',
  'IMG_tga.c',
  'IMG_tif.c',
  'IMG_webp.c',
  'IMG_WIC.c',
  'IMG_xcf.c',
  'IMG_xpm.c',
  'IMG_xv.c',
  'IMG_xxx.c',
)
