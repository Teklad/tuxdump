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
