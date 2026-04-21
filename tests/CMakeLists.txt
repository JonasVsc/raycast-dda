project("RaycastDDATest")

enable_testing()

# executable with 8kb required

add_executable(Test8KB "test_8kb.c")

target_link_libraries(Test8KB
	PRIVATE 
		RaycastDDA::RaycastDDA
)

add_test(NAME Test8KB COMMAND Test8KB)

# executable with scenarios test

add_executable(TestScenarios "test_scenarios.c")

target_link_libraries(TestScenarios
	PRIVATE 
		RaycastDDA::RaycastDDA
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

