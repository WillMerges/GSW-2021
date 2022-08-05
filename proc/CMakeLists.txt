add_executable(decom decom/src/decom.cpp)
add_executable(dlp dlp/src/dlp.cpp)
add_executable(mmon mmon/src/main.cpp)
add_executable(shmctl shmctl/src/shmctl.cpp)
add_executable(mqueue_test test/mqueue_test/src/test.cpp)
add_executable(shm_read_test test/shmtest/src/shmread.cpp)
add_executable(shm_write_test test/shmtest/src/shmwrite.cpp)
add_executable(vcm_test test/vcm_test/src/test.cpp)
add_executable(vlock_test test/vlock_test/src/main.cpp)
add_executable(log2csv tool/log2csv/src/main.cpp)
add_executable(log2influx tool/log2influx/src/main.cpp)
add_executable(log_ctrl tool/log_ctrl/src/main.cpp)
add_executable(mdns_publish tool/mdns_publish/src/main.cpp)
add_executable(uplink uplink/src/uplink.cpp)

target_link_libraries(decom telemetry nm vcm dls convert shm)
target_link_libraries(dlp dls shm rt)
target_link_libraries(mmon dls vcm telemetry trigger ec daq convert shm)
target_link_libraries(shmctl dls vcm telemetry nm convert shm clock vlock)

# TODO: Maybe make tests optionally compiled
target_link_libraries(mqueue_test dls shm rt)
target_link_libraries(shm_read_test vcm telemetry convert shm dls)
target_link_libraries(shm_write_test vcm telemetry convert shm dls)
target_link_libraries(vcm_test vcm dls shm rt)
target_link_libraries(vlock_test vlock shm dls rt)


target_link_libraries(log2csv vcm dls shm convert)
target_link_libraries(log2influx vcm dls shm convert)
target_link_libraries(log_ctrl dls shm rt)
target_link_libraries(uplink dls nm vcm shm)



#target_sources(${PROJECT_NAME} PRIVATE
#        decom/src/decom.cpp
#
#        dlp/src/dlp.cpp
#
#        mmon/src/main.cpp
#
#        shmctl/src/shmctl.cpp
#
#        test/mqueue_test/src/test.cpp
#        test/shmtest/src/shmread.cpp
#        test/shmtest/src/shmwrite.cpp
#        test/vcm_test/src/test.cpp
#        test/vlock_test/src/main.cpp
#
#        tool/log2csv/src/main.cpp
#        tool/log2influx/src/main.cpp
#        tool/log_ctrl/src/main.cpp
#        tool/mdns_publish/src/main.cpp
#
#        uplink/src/uplink.cpp
#        )