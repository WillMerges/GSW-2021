add_executable(clock clock/src/clock.cpp)
add_executable(convert convert/src/convert.cpp)
add_executable(sensors daq/src/sensors.cpp)
add_executable(serial dev/serial/src/serial.c)
add_executable(net dev/xbee/src/net.c)
add_executable(xbee dev/xbee/src/xbee.c)
add_executable(dl_shm dls/src/dl_shm.cpp)
add_executable(Logger dls/src/Logger.cpp)
add_executable(MsgLogger dls/src/MsgLogger.cpp)
add_executable(PacketLogger dls/src/PacketLogger.cpp)
add_executable(ec ec/src/ec.cpp)
add_executable(nm nm/src/nm.cpp)
add_executable(NmShm nm/src/NmShm.cpp)
add_executable(bindings python/bindings.cpp)
add_executable(shm shm/src/shm.cpp)
add_executable(TelemetryShm telemetry/src/TelemetryShm.cpp)
add_executable(TelemetryViewer telemetry/src/TelemetryViewer.cpp)
add_executable(TelemetryWriter telemetry/src/TelemetryWriter.cpp)
add_executable(basic trigger/src/basic.cpp)
add_executable(trigger trigger/src/trigger.cpp)
add_executable(vcm vcm/src/vcm.cpp)
add_executable(vlock vlock/src/vlock.cpp)

target_sources(
        clock/src/clock.cpp

        convert/src/convert.cpp

        daq/src/sensors.cpp

        dev/serial/src/serial.c
        dev/xbee/src/net.c
        dev/xbee/src/xbee.c

        dls/src/dl_shm.cpp
        dls/src/Logger.cpp
        dls/src/MsgLogger.cpp
        dls/src/PacketLogger.cpp

        ec/src/ec.cpp

        nm/src/nm.cpp
        nm/src/NmShm.cpp

        python/bindings.cpp

        shm/src/shm.cpp

        telemetry/src/TelemetryShm.cpp
        telemetry/src/TelemetryViewer.cpp
        telemetry/src/TelemetryWriter.cpp

        trigger/src/basic.cpp
        trigger/src/trigger.cpp

        vcm/src/vcm.cpp

        vlock/src/vlock.cpp
)