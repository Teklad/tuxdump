cmake_minimum_required(VERSION 3.6)

project(tuxdump VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(BuildType)

find_package(libconfig++ CONFIG QUIET)
find_package(PkgConfig QUIET)

# Disable some unused things in the subprojects
set(BUILD_TESTS OFF CACHE BOOL "Build libconfig tests")
set(BUILD_EXAMPLES OFF CACHE BOOL "Build libconfig examples")
set(FMT_TEST OFF CACHE BOOL "Disable fmt tests")

if (NOT CONFIG++_FOUND)
    if (PkgConfig_FOUND)
        pkg_check_modules(CONFIG++ libconfig++ QUIET)
    endif()
    if (NOT CONFIG++_FOUND)
	set(USE_INTERNAL_LIBCONFIG ON CACHE BOOL "Use internal libconfig" FORCE)
        add_subdirectory(libs/libconfig)
    endif()
else()
    set(CONFIG++_LIBRARY "${LIBCONFIG++_LIBRARIES}")
    set(CONFIG++_INCLUDE_DIR "${LIBCONFIG++_INCLUDE_DIRS}")
endif()

if (USE_INTERNAL_LIBCONFIG)
    message(STATUS "Using internal libconfig")
    set(CONFIG++_LIBRARY "config++")
    set(CONFIG++_INCLUDE_DIR "libs/libconfig/lib")
else()
    message(STATUS "Using system libconfig")
endif()

add_executable(${PROJECT_NAME}
    src/formatter.cpp
    src/main.cpp
    src/tools/signatures.cpp
    src/tools/netvars.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CONFIG++_INCLUDE_DIR}
    libs/rapidjson/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CONFIG++_LIBRARY}
    fmt
    tuxproc
)

configure_file("config/csgo.cfg" "${CMAKE_BINARY_DIR}/csgo.cfg" COPYONLY)
configure_file("config/formats.cfg" "${CMAKE_BINARY_DIR}/formats.cfg" COPYONLY)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_NAME="${PROJECT_NAME}")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)
target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
)

add_subdirectory(libs/tuxproc)
add_subdirectory(libs/fmt)
