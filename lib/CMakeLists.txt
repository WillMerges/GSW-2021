add_library(clock SHARED IMPORTED clock/src/clock.cpp)
add_library(convert SHARED IMPORTED convert/src/convert.cpp)
add_library(sensors SHARED IMPORTED daq/src/sensors.cpp)
add_library(serial SHARED IMPORTED dev/serial/src/serial.c)
add_library(net SHARED IMPORTED dev/xbee/src/net.c)
add_library(xbee SHARED IMPORTED dev/xbee/src/xbee.c)
add_library(dl_shm SHARED IMPORTED dls/src/dl_shm.cpp)
add_library(Logger SHARED IMPORTED dls/src/Logger.cpp)
add_library(MsgLogger SHARED IMPORTED dls/src/MsgLogger.cpp)
add_library(PacketLogger SHARED IMPORTED dls/src/PacketLogger.cpp)
add_library(ec SHARED IMPORTED ec/src/ec.cpp)
add_library(nm SHARED IMPORTED nm/src/nm.cpp)
add_library(NmShm SHARED IMPORTED nm/src/NmShm.cpp)
add_library(bindings SHARED IMPORTED python/bindings.cpp)
add_library(shm SHARED IMPORTED shm/src/shm.cpp)
add_library(TelemetryShm SHARED IMPORTED telemetry/src/TelemetryShm.cpp)
add_library(TelemetryViewer SHARED IMPORTED telemetry/src/TelemetryViewer.cpp)
add_library(TelemetryWriter SHARED IMPORTED telemetry/src/TelemetryWriter.cpp)
add_library(basic SHARED IMPORTED trigger/src/basic.cpp)
add_library(trigger SHARED IMPORTED trigger/src/trigger.cpp)
add_library(vcm SHARED IMPORTED vcm/src/vcm.cpp)
add_library(vlock SHARED IMPORTED vlock/src/vlock.cpp)

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