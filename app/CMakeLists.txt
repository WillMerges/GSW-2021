add_executable(clock_ctrl clock_ctrl/src/main.cpp   )
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