project("RaycastDDAVisualizer")

file(GLOB_RECURSE SOURCE LIST_DIRECTORIES false CONFIGURE_DEPENDS "src/*.c")

add_library(${PROJECT_NAME} STATIC ${SOURCE})
add_library(RaycastDDAVisualizer::RaycastDDAVisualizer ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "include")

