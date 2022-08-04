set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I../include -Wall -Wextra -Wpedantic -fpic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I../include -Wall -Wextra -Wpedantic -fpic -ggdb")

add_executable(clock clock/src/clock.cpp)
target_link_directories(clock)

add_executable(convert convert/src/convert.cpp)
target_link_directories(convert)

add_executable(sensors daq/src/sensors.cpp)
target_link_directories(sensors)

add_executable(serial dev/serial/src/serial.c)
target_link_directories(serial)

add_executable(net dev/xbee/src/net.c)
target_link_directories(net)

add_executable(xbee dev/xbee/src/xbee.c)
target_link_directories(xbee)

add_executable(dl_shm dls/src/dl_shm.cpp)
target_link_directories(dl_shm)

add_executable(Logger dls/src/Logger.cpp)
target_link_directories(Logger)

add_executable(MsgLogger dls/src/MsgLogger.cpp)
target_link_directories(MsgLogger)

add_executable(PacketLogger dls/src/PacketLogger.cpp)
target_link_directories(PacketLogger)

add_executable(ec ec/src/ec.cpp)
target_link_directories(ec)

add_executable(nm nm/src/nm.cpp)
target_link_directories(nm)

add_executable(NmShm nm/src/NmShm.cpp)
target_link_directories(NmShm)

add_executable(bindings python/bindings.cpp)
target_link_directories(bindings)

add_executable(shm shm/src/shm.cpp)
target_link_directories(shm)

add_executable(TelemetryShm telemetry/src/TelemetryShm.cpp)
target_link_directories(TelemetryShm)

add_executable(TelemetryViewer telemetry/src/TelemetryViewer.cpp)
target_link_directories(TelemetryViewer)

add_executable(TelemetryWriter telemetry/src/TelemetryWriter.cpp)
target_link_directories(TelemetryWriter)

add_executable(basic trigger/src/basic.cpp)
target_link_directories(basic)

add_executable(trigger trigger/src/trigger.cpp)
target_link_directories(trigger)

add_executable(vcm vcm/src/vcm.cpp)
target_link_directories(vcm)

add_executable(vlock vlock/src/vlock.cpp)
target_link_directories(vlock)

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