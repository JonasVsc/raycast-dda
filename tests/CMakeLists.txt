project("RaycastDDATest")

enable_testing()

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} 
	PRIVATE 
		RaycastDDA::RaycastDDA
)

add_test(NAME RaycastDDATest COMMAND ${PROJECT_NAME})