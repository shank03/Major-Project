#!/bin/bash

g++ udp_s.cpp -o cmake-build-debug/udp.out && sudo ./cmake-build-debug/udp.out ${1} ${2} ${3}
