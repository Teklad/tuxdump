cmake_minimum_required(VERSION 3.6)

project(tuxdump VERSION 1.0.0 LANGUAGES CXX)

find_package(TuxProc CONFIG REQUIRED)
find_package(libconfig++ CONFIG QUIET)
find_package(RapidJSON CONFIG

# Use pkg-config for libconfig++ if find_package fails
if (NOT LIBCONFIG++_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBCONFIG++ libconfig++ QUIET)
    # Build internal version if that fails as well
    if (NOT LIBCONFIG++_FOUND)
        set(USE_INTERNAL_LIBCONFIG ON CACHE BOOL "Use internal libconfig" FORCE)
        add_subdirectory(libs/libconfig)
    endif()
endif()

if (USE_INTERNAL_LIBCONFIG)
    message("Using internal libconfig")
    set(LIBCONFIG++_LIBRARIES "config++")
else()
    message("Using system libconfig: ${LIBCONFIG++_LIBRARIES} ${LIBCONFIG++_VERSION}")
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
target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
)

set(BUILD_TESTS OFF CACHE BOOL "Build libconfig tests" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build libconfig examples" FORCE)
set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)

add_subdirectory(libs/fmt)
add_subdirectory(libs/tuxproc)
