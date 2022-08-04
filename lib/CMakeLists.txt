add_library(clock clock/src/clock.cpp)
add_library(convert convert/src/convert.cpp)
add_library(sensors daq/src/sensors.cpp)
add_library(serial dev/serial/src/serial.c)
add_library(net dev/xbee/src/net.c)
add_library(xbee dev/xbee/src/xbee.c)
add_library(dl_shm dls/src/dl_shm.cpp)
add_library(Logger dls/src/Logger.cpp)
add_library(MsgLogger dls/src/MsgLogger.cpp)
add_library(PacketLogger dls/src/PacketLogger.cpp)
add_library(ec ec/src/ec.cpp)
add_library(nm nm/src/nm.cpp)
add_library(NmShm nm/src/NmShm.cpp)
add_library(bindings python/bindings.cpp)
add_library(shm shm/src/shm.cpp)
add_library(TelemetryShm telemetry/src/TelemetryShm.cpp)
add_library(TelemetryViewer telemetry/src/TelemetryViewer.cpp)
add_library(TelemetryWriter telemetry/src/TelemetryWriter.cpp)
add_library(basic trigger/src/basic.cpp)
add_library(trigger trigger/src/trigger.cpp)
add_library(vcm vcm/src/vcm.cpp)
add_library(vlock vlock/src/vlock.cpp)

#target_sources(
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