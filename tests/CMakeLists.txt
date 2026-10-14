add_executable(empire_tests
	main_test.cpp
	test_resources.cpp
	test_production.cpp
	test_army.cpp
	test_upgrades.cpp
	test_era.cpp
	test_save.cpp
	test_offline.cpp
	test_balance.cpp
	test_determinism.cpp
)

target_link_libraries(empire_tests PRIVATE
	empire::includes
	empire::warnings
	empire::engine::util
	empire::game::data
	empire::game::state
	empire::game::content
	empire::game::systems
	empire::game::logic
	empire::game::save
	Catch2::Catch2WithMain
)

target_compile_features(empire_tests PRIVATE cxx_std_20)

include(Catch)
catch_discover_tests(empire_tests)