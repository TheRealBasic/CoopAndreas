cmake_minimum_required(VERSION 3.20)

# Wrapper CMake project for root-level configuration.
# The primary project build is still driven by xmake (see xmake.lua).

set(COOPANDREAS_XMAKE_TARGET "server" CACHE STRING "xmake target to build (e.g. server, client, proxy)")
set(COOPANDREAS_XMAKE_MODE "release" CACHE STRING "xmake build mode")
option(COOPANDREAS_ENABLE_XMAKE_BRIDGE "Enable CMake targets that call xmake" ON)

find_program(COOPANDREAS_XMAKE_EXECUTABLE xmake)

add_custom_target(coopandreas-configure ALL
    COMMAND ${CMAKE_COMMAND} -E echo "CoopAndreas configured from repository root."
    COMMAND ${CMAKE_COMMAND} -E echo "Primary build system: xmake (xmake.lua)."
    COMMAND ${CMAKE_COMMAND} -E echo "Use: xmake --build ${COOPANDREAS_XMAKE_TARGET}"
    VERBATIM
)

if(COOPANDREAS_ENABLE_XMAKE_BRIDGE)
    if(COOPANDREAS_XMAKE_EXECUTABLE)
        add_custom_target(coopandreas-xmake
            COMMAND ${COOPANDREAS_XMAKE_EXECUTABLE} f -m ${COOPANDREAS_XMAKE_MODE}
            COMMAND ${COOPANDREAS_XMAKE_EXECUTABLE} --build ${COOPANDREAS_XMAKE_TARGET}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Building CoopAndreas via xmake bridge"
            VERBATIM
        )
    else()
        message(STATUS "xmake was not found; target 'coopandreas-xmake' is not available.")
    endif()
endif()
