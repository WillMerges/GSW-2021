cmake_minimum_required(VERSION 3.15)
project(launch_gsw)
set(CMAKE_CXX_STANDARD 14)
set(GSW_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LIB_DIR lib)
set(APP_DIR app)
set(PROC_DIR proc)
set(SIM_DIR sim)

include_directories(include)
#target_include_directories(${proj_name} ${CMAKE_CURRENT_SOURCE_DIR}/include/> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
#target_include_directories(include)

add_subdirectory(lib)
#add_subdirectory(proc)
#add_subdirectory(app)
#add_subdirectory(sim)

