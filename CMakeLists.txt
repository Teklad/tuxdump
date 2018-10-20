cmake_minimum_required(VERSION 3.6)

project(tuxdump VERSION 1.0.0 LANGUAGES CXX)

find_package(TuxProc CONFIG REQUIRED)
find_package(libconfig++ CONFIG QUIET)

# Use pkg-config for libconfig++ if find_package fails
if (NOT LIBCONFIG++_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBCONFIG++ REQUIRED libconfig++)
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/printer.cpp
    src/tools/signatures.cpp
    src/tools/netvars.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIBCONFIG++_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    TuxProc::tuxproc
    ${LIBCONFIG++_LIBRARIES}
    fmt
)

configure_file("config/csgo.cfg" "${CMAKE_BINARY_DIR}/csgo.cfg" COPYONLY)
configure_file("config/formats.cfg" "${CMAKE_BINARY_DIR}/formats.cfg" COPYONLY)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_NAME="${PROJECT_NAME}")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>)

add_subdirectory(libs/fmt)
add_subdirectory(libs/tuxproc)
