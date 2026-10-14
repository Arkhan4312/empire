# GameLogic.h/.cpp
empire_add_module(game_logic
	STATIC
	ALIAS empire::game::logic
	SOURCES
		GameLogic.h
		GameLogic.cpp
	PUBLIC
		empire::includes
		empire::game::state
		empire::game::systems
		empire::engine::core
)