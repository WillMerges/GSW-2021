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