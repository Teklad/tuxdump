cmake_minimum_required(VERSION 3.2.0)
project(tuxdump-csgo VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

find_package(libconfig REQUIRED)

if (CMAKE_BUILD_TYPE EQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
endif()

add_executable(tuxdump
    tuxdump.c
    main.c
)

configure_file(csgo.cfg csgo.cfg COPYONLY)

target_link_libraries(tuxdump
    ${LIBCONFIG_LIBRARIES}
    )
