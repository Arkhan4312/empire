#audioSystem.h/.cpp miniaudio_impl.cpp
empire_add_module(engine_audio
	STATIC
	ALIAS empire::engine::audio
	SOURCES
		AudioSystem.h
		AudioSystem.cpp
		miniaudio_impl.cpp
	PUBLIC
		empire::includes
	PRIVATE
		miniaudio_headers
)