cmake_minimum_required(VERSION 3.15)
project(launch_gsw)
set(CMAKE_CXX_STANDARD 14)
set(GSW_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_C_FLAGS "-I${GSW_DIR}/include -Wall -Wextra -Wpedantic -ggdb")
set(CMAKE_CXX_FLAGS "-I${GSW_DIR}/include -Wall -Wextra -Wpedantic -ggdb")


set(SOURCES)
set(INC_DIR include)
set(LIB_DIR lib)
set(APP_DIR app)
set(PROC_DIR proc)
set(SIM_DIR sim)

#file(GLOB_RECURSE
#        ALL_CXX_SOURCE_FILES
#        ${CMAKE_SOURCE_DIR}/include/*.[ch]pp  ${CMAKE_SOURCE_DIR}/include/*.[CH]
#        ${CMAKE_SOURCE_DIR}/lib/*.[ch]pp      ${CMAKE_SOURCE_DIR}/lib/*.[CH]
#        ${CMAKE_SOURCE_DIR}/app/*.[ch]pp  ${CMAKE_SOURCE_DIR}/app/*.[CH]
#        ${CMAKE_SOURCE_DIR}/proc/*.[ch]pp  ${CMAKE_SOURCE_DIR}/proc/*.[CH]
#        ${CMAKE_SOURCE_DIR}/sim/*.[ch]pp  ${CMAKE_SOURCE_DIR}/sim/*.[CH]
#        )

include_directories(include)

add_subdirectory(lib)

add_subdirectory(proc)
#add_subdirectory(app)
#add_subdirectory(sim)
