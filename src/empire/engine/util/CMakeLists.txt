# AssetPath.h/.cpp Result.h Handle.h Random.h

empire_add_module(engine_util
	STATIC
	ALIAS empire::engine::util
	SOURCES
		Random.h
		Result.h
		Handle.h
		AssetPath.h
		AssetPath.cpp
	PUBLIC
		empire::includes
)