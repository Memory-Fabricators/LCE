find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Dawn CONFIG REQUIRED)

add_executable(Minecraft.Client WIN32)

target_include_directories(Minecraft.Client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL3
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL3/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL3Media
)

file(GLOB CLIENT_ROOT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE CLIENT_COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/*.cpp"
)

file(GLOB_RECURSE CLIENT_SDL3_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/SDL3/*.cpp"
)

# TODO: Common/UI depends on the proprietary Iggy UI/font middleware, which has
# no SDL3 equivalent yet. Excluded until UI rendering is reimplemented for SDL3.
list(FILTER CLIENT_COMMON_SOURCES EXCLUDE REGEX "/Common/UI/")
# Consoles_App.cpp unconditionally includes UI.h (Iggy) and Xbox-specific headers
# with no platform guards - same Iggy dependency as Common/UI, excluded alongside it.
list(FILTER CLIENT_COMMON_SOURCES EXCLUDE REGEX "/Common/Consoles_App.cpp")
# TODO: SoundEngine.cpp is written entirely against the Miles Sound System (RAD Game
# Tools), with no SDL3/OpenAL branch. Needs a real OpenAL-backed reimplementation,
# same as the renderer - excluded until that's done.
list(FILTER CLIENT_COMMON_SOURCES EXCLUDE REGEX "/Common/Audio/SoundEngine.cpp")
list(REMOVE_ITEM CLIENT_ROOT_SOURCES
    # Not part of the real project (absent from Minecraft.Client.vcxproj) - dead code
    # left over in the tree, calling APIs with stale signatures.
    "${CMAKE_CURRENT_SOURCE_DIR}/CreativeMode.cpp"
)
# Sony PSN-specific network code (sce toolkit, np.h) - only built for PSVita/Orbis
# per Minecraft.Client.vcxproj, excluded on every other platform including this one.
list(FILTER CLIENT_COMMON_SOURCES EXCLUDE REGEX "/Common/Network/Sony/")
# Common/XUI is the Xbox360-era XUI framework (XUIMessageInit, CXuiSceneImpl, etc.) -
# only built for Xbox 360 per the vcxproj, excluded everywhere else including here.
list(FILTER CLIENT_COMMON_SOURCES EXCLUDE REGEX "/Common/XUI/")
# Pulls in Common/UI/UIBitmapFont.h (Iggy) - same dependency gap as Common/UI.
list(FILTER CLIENT_COMMON_SOURCES EXCLUDE REGEX "/Common/Tutorial/XuiCraftingTask.cpp")

target_sources(Minecraft.Client PRIVATE
    ${CLIENT_ROOT_SOURCES}
    ${CLIENT_COMMON_SOURCES}
    ${CLIENT_SDL3_SOURCES}
    # Platform-agnostic despite its name/location (a generic INetworkPlayer
    # wrapper around IQNetPlayer, no Xbox-specific dependencies) - reused by
    # PlatformNetworkManagerStub.cpp (the non-console networking stub) on
    # every platform without its own PlatformNetworkManager, SDL3 included.
    ${CMAKE_CURRENT_SOURCE_DIR}/Xbox/Network/NetworkPlayerXbox.cpp
    # Just a hardcoded codepoint data table (SFontData) - no Iggy dependency,
    # unlike the rest of Common/UI, so it's fine to build despite the blanket
    # Common/UI/ exclusion below.
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/UI/UIFontData.cpp
)

target_link_libraries(Minecraft.Client PRIVATE
    Minecraft.World
    SDL3::SDL3
    SDL3_image::SDL3_image
    PNG::PNG
    ZLIB::ZLIB
    dawn::webgpu_dawn
)

# Dawn's webgpu_cpp.h requires C++17 (std::optional, etc.); the rest of this
# legacy codebase targets C++11, so scope the newer standard to just the file
# that includes Dawn headers rather than bumping the whole project.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL3/4JLibs/src/4J_Render.cpp
    PROPERTIES COMPILE_OPTIONS "-std=c++20"
)
