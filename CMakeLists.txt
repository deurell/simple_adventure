cmake_minimum_required(VERSION 3.26)
project(adventure)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(adventure main.cpp)

configure_file(${CMAKE_SOURCE_DIR}/resources/map.json ${CMAKE_BINARY_DIR}/map.json COPYONLY)
