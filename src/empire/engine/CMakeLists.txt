add_subdirectory(core)
add_subdirectory(log)
add_subdirectory(util)
add_subdirectory(settings)
add_subdirectory(render)
add_subdirectory(ui)
add_subdirectory(audio)

add_library(empire_engine INTERFACE)
add_library(empire::engine ALIAS empire_engine)
target_link_libraries(empire_engine INTERFACE
	empire::engine::core
	empire::engine::log
	empire::engine::util
	empire::engine::settings
	empire::engine::render
	empire::engine::ui
	empire::engine::audio)