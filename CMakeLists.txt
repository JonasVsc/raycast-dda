cmake_minimum_required(VERSION 3.20)

project("RaycastDDA"
        VERSION 0.1
        DESCRIPTION "Raycast using DDA"
        LANGUAGES C)

enable_testing()

add_subdirectory(raycast-dda)
add_subdirectory(tests)