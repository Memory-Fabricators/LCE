cmake_minimum_required(VERSION 3.24)

project(LCE LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ZLIB REQUIRED)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(SDL3_mixer REQUIRED)
find_package(Vulkan REQUIRED)

option(ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)

if(ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-p;${CMAKE_CURRENT_BINARY_DIR}")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/WorldSources.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ClientSources.cmake")

list(TRANSFORM MINECRAFT_WORLD_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.World/")
list(TRANSFORM MINECRAFT_CLIENT_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/")
# list(APPEND MINECRAFT_CLIENT_SOURCES
#   "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Xbox/MinecraftWindows.rc"
# )

add_executable(MinecraftClient ${MINECRAFT_CLIENT_SOURCES} ${MINECRAFT_WORLD_SOURCES})
target_include_directories(MinecraftClient PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.World"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Xbox/Sentient/Include"
)
target_link_libraries(MinecraftClient PRIVATE
  SDL3::SDL3
  ZLIB::ZLIB
  Vulkan::Vulkan
  MinecraftWorld
)
target_compile_features(MinecraftClient PRIVATE
  cxx_std_26
)
