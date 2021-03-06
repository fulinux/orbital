cmake_minimum_required(VERSION 2.8)
project(orbital)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -Werror=return-type")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED true)

include(Wayland)
find_package(PkgConfig "0.22" REQUIRED)

add_definitions(-DQT_NO_CAST_FROM_ASCII
                -DQT_NO_CAST_TO_ASCII
                -DQT_NO_CAST_FROM_BYTEARRAY
                -DQT_NO_URL_CAST_FROM_STRING
                -DQT_USE_QSTRINGBUILDER)

add_subdirectory(src)

enable_testing()
add_custom_target(check ${CMAKE_CTEST_COMMAND} -V)
add_subdirectory(test EXCLUDE_FROM_ALL)

# uninstall target
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

add_subdirectory(scripts)
add_subdirectory(data)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/orbital-launcher.desktop DESTINATION /etc/xdg/autostart)
