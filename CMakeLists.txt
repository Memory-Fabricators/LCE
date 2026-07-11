cmake_minimum_required(VERSION 4.0)
project(lce
  VERSION 0.1.0
  LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_UNITY_BUILD OFF)

# Options
option(LCE_DEBUG_MENUS "Enable debug menus" ON)
option(LCE_LARGE_WORLDS "Enable large worlds" ON)
option(LCE_SPLIT_SAVES "Enable split saves" ON)

# Platform defines
add_compile_definitions(
  _SDL3
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

if(LCE_DEBUG_MENUS)
    add_compile_definitions(_DEBUG_MENUS_ENABLED)
endif()
if(LCE_LARGE_WORLDS)
    add_compile_definitions(_LARGE_WORLDS)
endif()
if(LCE_SPLIT_SAVES)
    add_compile_definitions(SPLIT_SAVES)
endif()

add_subdirectory(Minecraft.World)
add_subdirectory(Minecraft.Client)
