project("RaycastDDATest")

# executable with 8kb required also output image

add_executable(Test8KB "test_8kb.c")

target_link_libraries(Test8KB
	PRIVATE 
		RaycastDDA::RaycastDDA
		RaycastDDAVisualizer::RaycastDDAVisualizer
)

if(MSVC)
   target_link_options(Test8KB 
		PRIVATE 
			/MAP
	)
endif()

add_test(NAME Test8KB COMMAND Test8KB)

# executable with scenarios test also output scenarios images

add_executable(TestScenarios "test_scenarios.c")

target_link_libraries(TestScenarios
	PRIVATE 
		RaycastDDA::RaycastDDA
		RaycastDDAVisualizer::RaycastDDAVisualizer
)

add_test(NAME TestScenarios COMMAND TestScenarios)

# executable with scenarios lodev implementation

add_executable(TestScenariosLodev "test_lodev.cpp")

target_link_libraries(TestScenariosLodev
	PRIVATE 
		RaycastDDA::RaycastDDA
)

add_test(NAME TestScenariosLodev COMMAND TestScenariosLodev)

# executable with scenarios comparison ( our scenarios == lodev scenarios )

add_executable(TestComparison "test_comparison.cpp")

add_test(NAME TestComparison COMMAND TestComparison)
set_tests_properties(TestComparison PROPERTIES
    DEPENDS "TestScenarios;TestScenariosLodev"
)