cmake_minimum_required(VERSION 3.2.0)
project(tuxdump VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -lconfig++")

if (CMAKE_BUILD_TYPE EQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

configure_file(config/csgo.cfg csgo.cfg COPYONLY)

add_subdirectory(src)
