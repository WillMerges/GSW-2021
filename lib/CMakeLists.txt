add_library(clock SHARED STATIC clock/src/clock.cpp)
add_library(convert SHARED STATIC convert/src/convert.cpp)
add_library(sensors SHARED STATIC daq/src/sensors.cpp)
add_library(serial SHARED STATIC dev/serial/src/serial.c)
add_library(xbee SHARED STATIC dev/xbee/src/xbee.c dev/xbee/src/net.c)
add_library(dls SHARED STATIC dls/src/dl_shm.cpp dls/src/Logger.cpp dls/src/MsgLogger.cpp dls/src/PacketLogger.cpp)
add_library(ec SHARED STATIC ec/src/ec.cpp)
add_library(nm SHARED STATIC nm/src/nm.cpp nm/src/NmShm.cpp)
add_library(bindings SHARED STATIC python/bindings.cpp)
add_library(shm SHARED STATIC shm/src/shm.cpp)
add_library(telemetry SHARED STATIC telemetry/src/TelemetryShm.cpp telemetry/src/TelemetryViewer.cpp telemetry/src/TelemetryWriter.cpp)
add_library(basic SHARED STATIC trigger/src/basic.cpp)
add_library(trigger SHARED STATIC trigger/src/trigger.cpp)
add_library(vcm SHARED STATIC vcm/src/vcm.cpp)
add_library(vlock SHARED STATIC vlock/src/vlock.cpp)

target_link_libraries(clock convert sensors serial xbee
        dls ec nm bindings shm telemetry basic trigger vcm vlock)

set(SOURCES )


#target_sources(${PROJECT_NAME} PRIVATE
#        clock/src/clock.cpp
#
#        convert/src/convert.cpp
#
#        daq/src/sensors.cpp
#
#        dev/serial/src/serial.c
#        dev/xbee/src/net.c
#        dev/xbee/src/xbee.c
#
#        dls/src/dl_shm.cpp
#        dls/src/Logger.cpp
#        dls/src/MsgLogger.cpp
#        dls/src/PacketLogger.cpp
#
#        ec/src/ec.cpp
#
#        nm/src/nm.cpp
#        nm/src/NmShm.cpp
#
#        python/bindings.cpp
#
#        shm/src/shm.cpp
#
#        telemetry/src/TelemetryShm.cpp
#        telemetry/src/TelemetryViewer.cpp
#        telemetry/src/TelemetryWriter.cpp
#
#        trigger/src/basic.cpp
#        trigger/src/trigger.cpp
#
#        vcm/src/vcm.cpp
#
#        vlock/src/vlock.cpp
#)