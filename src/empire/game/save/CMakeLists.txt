# SaveSystem.h/.cpp

empire_add_module(game_save
	STATIC
	ALIAS empire::game::save
	SOURCES
		SaveSystem.h
		SaveSystem.cpp
	PUBLIC
		empire::includes
		empire::game::state
		empire::game::content
	PRIVATE
		empire::game::logic
		nlohmann_json::nlohmann_json
)