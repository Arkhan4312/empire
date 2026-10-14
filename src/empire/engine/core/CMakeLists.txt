#Input.h InputSystem.h/.cpp Tick.h 

empire_add_module(engine_core
	STATIC
	ALIAS empire::engine::core
	SOURCES
		Input.h
		InputSystem.h
		InputSystem.cpp
		Tick.h
	PUBLIC
		empire::includes
		glm::glm
	PRIVATE
		glfw
)