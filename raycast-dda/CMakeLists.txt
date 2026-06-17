project("RaycastDDA")

file(GLOB_RECURSE SOURCE LIST_DIRECTORIES false CONFIGURE_DEPENDS "src/*.c")

add_library(${PROJECT_NAME} STATIC ${SOURCE})
add_library(RaycastDDA::RaycastDDA ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "include")

if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE /MAP)
endif()
