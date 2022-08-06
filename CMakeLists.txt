project(launch_gsw)

cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 14)
set(GSW_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_C_FLAGS "-I${GSW_DIR}/include -Wall -Wextra -Wpedantic -ggdb")
set(CMAKE_CXX_FLAGS "-I${GSW_DIR}/include -Wall -Wextra -Wpedantic -ggdb")
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_library(LIBRT rt)

set(SOURCES)
set(INC_DIR include)
set(LIB_DIR lib)
set(APP_DIR app)
set(PROC_DIR proc)
set(SIM_DIR sim)


include_directories(include)
include_directories(${THREADS_INCLUDE_DIR})


add_subdirectory(lib)
add_subdirectory(proc)
add_subdirectory(app)
add_subdirectory(sim)