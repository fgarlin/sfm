cmake_minimum_required(VERSION 2.8)
project(sfm)

set(CMAKE_BUILD_TYPE Debug)

# 3rd party libraries
include(cmake/3rdparty.cmake)

find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(src)
