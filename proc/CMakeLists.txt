add_executable(decom decom/src/decom.cpp)
add_executable(dlp dlp/src/dlp.cpp)
add_executable(mmon mmon/src/main.cpp)
add_executable(shmctl shmctl/src/shmctl.cpp)
add_executable(mqueue_test test/mqueue_test/src/test.cpp)
add_executable(shmread test/shmtest/src/shmread.cpp)
add_executable(shmwrite test/shmtest/src/shmwrite.cpp)
add_executable(vcm_test test/vcm_test/src/test.cpp)
add_executable(vlock_test test/vlock_test/src/main.cpp)
add_executable(log2csv tool/log2csv/src/main.cpp)
add_executable(log2influx tool/log2influx/src/main.cpp)
add_executable(log_ctrl tool/log_ctrl/src/main.cpp)
add_executable(mdns_publish tool/mdns_publish/src/main.cpp)
add_executable(uplink uplink/src/uplink.cpp)

target_sources(${PROJECT_NAME} PRIVATE
        decom/src/decom.cpp

        dlp/src/dlp.cpp

        mmon/src/main.cpp

        shmctl/src/shmctl.cpp

        test/mqueue_test/src/test.cpp
        test/shmtest/src/shmread.cpp
        test/shmtest/src/shmwrite.cpp
        test/vcm_test/src/test.cpp
        test/vlock_test/src/main.cpp

        tool/log2csv/src/main.cpp
        tool/log2influx/src/main.cpp
        tool/log_ctrl/src/main.cpp
        tool/mdns_publish/src/main.cpp

        uplink/src/uplink.cpp
        )