# Scene SceneManager AppContext MainMenuScene GameScene SettingsScene

empire_add_module(game_scenes
	STATIC
	ALIAS empire::game::scenes
	SOURCES
		Scene.h
		SceneManager.h
		SceneManager.cpp
		AppContext.h
		MainMenuScene.h
		MainMenuScene.cpp
		GameScene.h
		GameScene.cpp
		SettingsScene.h
		SettingsScene.cpp
	PUBLIC
		empire::includes
		empire::engine::core
		empire::engine::render
		empire::engine::ui
		empire::engine::audio
		empire::engine::settings
		empire::engine::log
		empire::game::data
		empire::game::state
		empire::game::content
		empire::game::systems
		empire::game::logic
		empire::game::save
)