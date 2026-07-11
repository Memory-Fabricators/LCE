add_library(Minecraft.World STATIC)

find_package(SDL3 REQUIRED)

target_include_directories(Minecraft.World PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/x64headers
    ${CMAKE_SOURCE_DIR}/Minecraft.Client/SDL3
    ${CMAKE_SOURCE_DIR}/Minecraft.Client/SDL3/stubs
)

target_link_libraries(Minecraft.World PUBLIC
    SDL3::SDL3
)

file(GLOB WORLD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

list(REMOVE_ITEM WORLD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/ZonedChunkStorage.cpp"
)

target_sources(Minecraft.World PRIVATE ${WORLD_SOURCES})
