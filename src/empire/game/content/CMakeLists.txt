#content.h/.cpp

empire_add_module(game_content
	STATIC
	ALIAS empire::game::content
	SOURCES
		Content.h
		Content.cpp
	PUBLIC
		empire::includes
		empire::game::state
		empire::game::data
	PRIVATE
		nlohmann_json::nlohmann_json
)