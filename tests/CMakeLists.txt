add_executable(empire_tests
	main_test.cpp
	audio/test_audio_bus_config.cpp
	audio/test_audio_bus_registry.cpp
	audio/test_audio_device_manager.cpp
	audio/test_audio_error.cpp
	audio/test_audio_event_log.cpp
	audio/test_audio_integration_null_engine.cpp
	audio/test_audio_music_player.cpp
	audio/test_audio_resource.cpp
	audio/test_audio_resource_cache.cpp
	audio/test_audio_system.cpp
	audio/test_audio_voice_pool.cpp
	audio/test_audio_raii.cpp
	util/test_handle.cpp
	util/test_result.cpp
	util/test_assetPath.cpp
	log/test_log.cpp
	events/test_events.cpp
	game/test_resources.cpp
	game/test_production.cpp
	game/test_army.cpp
	game/test_upgrades.cpp
	game/test_era.cpp
	game/test_save.cpp
	game/test_offline.cpp
	game/test_balance.cpp
	game/test_determinism.cpp
	game/test_asset_path.cpp
	game/test_result.cpp
	game/test_handle.cpp
)

target_link_libraries(empire_tests PRIVATE
	empire::includes
	empire::warnings
	empire::engine::util
	empire::engine::audio
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