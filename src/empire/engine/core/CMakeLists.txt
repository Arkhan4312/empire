#Input.h InputSystem.h/.cpp Tick.h 

empire_add_module(engine_core
	STATIC
	ALIAS empire::engine::core
	SOURCES
		events/Event.h
		events/EventBus.h		events/EventBus.cpp
		Input.h
		InputSystem.h
		InputSystem.cpp
		Tick.h
	PUBLIC
		empire::includes
		empire::engine::util
		glm::glm
	PRIVATE
		glfw
)