cmake_minimum_required(VERSION 3.22)
project(tcp)

set(CMAKE_CXX_STANDARD 17)

add_executable(tcp main.cpp
        csm.h
        netfilter.h
        checksum.h
)

set_target_properties(tcp PROPERTIES OUTPUT_NAME tcp-shank)

target_link_libraries(tcp -lnfnetlink -lnetfilter_queue)
