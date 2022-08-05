find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})
find_package(Threads REQUIRED)
include_directories(${THREADS_INCLUDE_DIR}) # TODO: make this cleaner and global

add_executable(clock_ctrl clock_ctrl/src/main.cpp)
add_executable(clock_view clock_view/src/main.cpp)
add_executable(fwd_influx db/fwd_influx/src/main.cpp)
add_executable(log_view log_view/src/main.cpp)
add_executable(mem_view mem_view/src/main.cpp)
add_executable(nview nview/src/main.cpp)
add_executable(print_meas print_meas/src/main.cpp)
add_executable(ec_cmd propulsion/ec_cmd/src/main.cpp)
add_executable(ec_program propulsion/ec_program/src/main.cpp)
add_executable(val_view val_view/src/main.cpp)
add_executable(voice_report voice/src/voice_report.cpp)
add_executable(darwin xbee/darwin/src/main.cpp)
add_executable(xbee_bridge xbee/xbee_bridge/src/main.cpp)

# TODO: Figure out what to do with py files
target_link_libraries(clock_ctrl dls vcm telemetry convert clock shm)
target_link_libraries(clock_view dls vcm telemetry convert clock shm)
target_link_libraries(fwd_influx dls vcm telemetry clock shm convert ${THREADS_INCLUDE_DIR})
target_link_libraries(log_view dls shm)
target_link_libraries(mem_view dls telemetry convert shm vcm)
target_link_libraries(nview telemetry dls vcm convert shm ${THREADS_INCLUDE_DIR} ${CURSES_LIBRARIES})
target_link_libraries(print_meas dls vcm telemetry shm convert)
target_link_libraries(val_view dls vcm telemetry convert shm)
target_link_libraries(voice_report dls vcm telemetry shm convert)

#target_sources(
#        clock_ctrl/src/main.cpp
#        clock_view/src/main.cpp
#
#        db/fwd_influx/src/main.cpp
#
#        log_view/src/main.cpp
#
#        mem_view/src/main.cpp
#
#        nview/src/main.cpp
#
#        print_meas/src/main.cpp
#
#        propulsion/ec_cmd/src/main.cpp
#        propulsion/ec_program/src/main.cpp
#
#        val_view/src/main.cpp
#
#        voice/src/voice_report.cpp
#
#        xbee/darwin/src/main.cpp
#        xbee/xbee_bridge/src/main.cpp
#)