cmake_minimum_required(VERSION 3.15)
project(launch_gsw)
set(CMAKE_CXX_STANDARD 14)
set(GSW_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(include)
target_include_directories(include)

add_subdirectory(app)
add_subdirectory(lib)
add_subdirectory(proc)
add_subdirectory(sim)

