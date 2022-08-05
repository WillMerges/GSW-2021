find_package(Threads REQUIRED)
find_library(LIBRT rt)

add_library(clock SHARED clock/src/clock.cpp)
add_library(convert SHARED convert/src/convert.cpp)
add_library(daq SHARED daq/src/sensors.cpp)
add_library(serial SHARED dev/serial/src/serial.c)
add_library(xbee SHARED dev/xbee/src/xbee.c dev/xbee/src/net.c)
add_library(dls SHARED dls/src/dl_shm.cpp dls/src/Logger.cpp dls/src/MsgLogger.cpp dls/src/PacketLogger.cpp)
add_library(ec SHARED ec/src/ec.cpp)
add_library(nm SHARED nm/src/nm.cpp nm/src/NmShm.cpp)
add_library(bindings SHARED python/bindings.cpp)
add_library(shm SHARED shm/src/shm.cpp)
add_library(telemetry SHARED telemetry/src/TelemetryShm.cpp telemetry/src/TelemetryViewer.cpp telemetry/src/TelemetryWriter.cpp)
add_library(trigger SHARED trigger/src/trigger.cpp trigger/src/basic.cpp)
add_library(vcm SHARED vcm/src/vcm.cpp)
add_library(vlock SHARED vlock/src/vlock.cpp)

target_link_libraries(dls rt)
target_link_libraries(ec rt)
target_link_libraries(nm rt)
target_link_libraries(shm Threads::Threads rt)

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