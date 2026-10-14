#windowMode.h settings.h SettingsSystem.h/.cpp KeyBindings.h

empire_add_module(engine_settings
	STATIC
	ALIAS empire::engine::settings
	SOURCES
		WindowMode.h
		Settings.h
		SettingsSystem.h
		SettingsSystem.cpp
		KeyBindings.h
	PUBLIC 	empire::includes
		empire::engine::core
	PRIVATE
		nlohmann_json::nlohmann_json
)