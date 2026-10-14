#Window Shader Texture SpriteBatch Font Renderer stb_impl.cpp

empire_add_module(engine_render
	STATIC
	ALIAS  empire::engine::render
	SOURCES
		Window.h
		Window.cpp
		Shader.h
		Shader.cpp
		Texture.h
		Texture.cpp
		SpriteBatch.h
		SpriteBatch.cpp
		Font.h
		Font.cpp
		Renderer.h
		Renderer.cpp
		stb_impl.cpp
	PUBLIC
		empire::includes
		empire::engine::log
		empire::engine::settings
		glm::glm
	PRIVATE
		glfw
		glad_gl_core_43
		stb_headers
)

target_compile_definitions(engine_render PUBLIC
	EMPIRE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
