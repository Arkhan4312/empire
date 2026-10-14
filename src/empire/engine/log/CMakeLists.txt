#logger.h/.cpp LogSink.h/.cpp

empire_add_module(engine_log
	STATIC
	ALIAS empire::engine::log
	SOURCES
		Logger.h
		Logger.cpp
		LogSink.h
		LogSink.cpp
	PUBLIC
		empire::includes
)