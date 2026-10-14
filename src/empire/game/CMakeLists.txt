add_subdirectory(data)
add_subdirectory(state)
add_subdirectory(content)
add_subdirectory(systems)
add_subdirectory(logic)
add_subdirectory(save)
add_subdirectory(scenes)

add_library(empire_game INTERFACE)
add_library(empire::game ALIAS empire_game)
target_link_libraries(empire_game INTERFACE
	empire::game::data
	empire::game::state
	empire::game::content
	empire::game::systems
	empire::game::logic
	empire::game::save
	empire::game::scenes
)