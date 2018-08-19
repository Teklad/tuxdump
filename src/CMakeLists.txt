add_executable(tuxdump
    main.cpp
    memory.cpp
    config.cpp
)

target_link_libraries(tuxdump
    ${LIBCONFIG++_LIBRARIES}
)

set_target_properties(tuxdump
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG_BUILD 1)
endif()

configure_file(defines.h.in defines.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

