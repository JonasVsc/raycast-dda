cmake_minimum_required(VERSION 3.20)

project("RaycastDDAAlgorithm"
        VERSION 0.1
        DESCRIPTION "Raycast using DDA"
        LANGUAGES C)

enable_testing()

set(CMAKE_BUILD_TYPE Release)

add_subdirectory(raycast-dda)
add_subdirectory(raycast-dda-visualizer)
add_subdirectory(tests)