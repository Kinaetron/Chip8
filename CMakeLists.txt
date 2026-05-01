cmake_minimum_required(VERSION 3.20)
project(Chip8 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL)

include(cmake/GenerateCimguiBindings.cmake)

set(SDL3_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include)
GenerateCimguiBindings(cimgui_gpu sdl3 sdlgpu3 SDL3_INCLUDES)

target_link_libraries(cimgui_gpu PRIVATE SDL3::SDL3)

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
    "shader/source/*"
    "shader/compiled/*"
)

add_executable(Chip8 
    src/main.c
    src/chip8.c
    src/shader.c 
    include/chip8.h
    include/shader.h
    ${SHADER_FILES}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    src/main.c 
    src/chip8.c
    src/shader.c 
    include/chip8.h
    include/shader.h
    ${SHADER_FILES}
)

target_include_directories(Chip8 PRIVATE include)

target_link_libraries(Chip8 
    PRIVATE 
        SDL3::SDL3 
        cimgui_gpu
)

target_compile_definitions(Chip8 PRIVATE 
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1
    CIMGUI_USE_SDL3=1
    CIMGUI_USE_SDLGPU3=1
)

add_custom_command(TARGET Chip8 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:Chip8>/shaders"
    COMMENT "Syncing shader/source and shader/compiled to build directory..."
)