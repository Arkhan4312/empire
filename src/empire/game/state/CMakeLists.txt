#gamestate.h/.cpp

empire_add_module(game_state
	STATIC
	ALIAS empire::game::state
	SOURCES
		GameState.h
		GameState.cpp
	PUBLIC
		empire::includes
		empire::game::data
)