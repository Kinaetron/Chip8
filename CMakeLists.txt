cmake_minimum_required(VERSION 4.0)
set(CMAKE_C_STANDARD 99)

project(Chip8)

add_subdirectory(external/SDL)
add_executable(Chip8 src/main.c)

# Statically link SDL3
set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)

target_link_libraries(Chip8 PRIVATE SDL3::SDL3)