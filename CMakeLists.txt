cmake_minimum_required(VERSION 4.0)

cmake_policy(SET CMP0157 NEW)
project(LCE LANGUAGES C CXX ASM Swift)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ZLIB REQUIRED)
find_package(Vulkan REQUIRED)

add_compile_options(-gline-tables-only)

add_executable(MinecraftClient
  Client/Client.swift
  Client/Window.swift
  World/Vec3.swift
)

if(APPLE)
  target_sources(MinecraftClient PRIVATE
    Client/Render/Renderer.swift
  )
endif()

target_include_directories(MinecraftClient PRIVATE Vulkan)

target_link_libraries(MinecraftClient PRIVATE
  ZLIB::ZLIB
  Vulkan::Vulkan
)
target_compile_features(MinecraftClient PRIVATE
  cxx_std_20
)
target_compile_options(MinecraftClient PRIVATE -cxx-interoperability-mode=default)
set_target_properties(MinecraftClient PROPERTIES 
  Swift_COMPILATION_MODE "$<$<CONFIG:Release>:wholemodule>$<$<NOT:$<CONFIG:Release>>:incremental>"
)