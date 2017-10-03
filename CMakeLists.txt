cmake_minimum_required(VERSION 3.8)
project(lingi1341_project)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES src/sender.c src/packet_interface.h src/packet_implem.c)
add_executable(lingi1341_project ${SOURCE_FILES})
