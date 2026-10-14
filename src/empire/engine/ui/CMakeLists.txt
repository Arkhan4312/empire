# Layout.h Widget.h UIContext.h/.cpp Widgets.h/.cpp

empire_add_module(engine_ui
	STATIC
	ALIAS empire::engine::ui
	SOURCES
		UIContext.cpp
		Widgets.cpp
	PUBLIC
		empire::includes
		empire::engine::core
		empire::engine::render
		glm::glm
	PRIVATE
		glad_gl_core_43
)