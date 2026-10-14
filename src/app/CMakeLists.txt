add_executable(empire main.cpp)

target_link_libraries(empire PRIVATE
	empire::includes
	empire::warnings
	empire::engine
	empire::game
)

target_compile_features(empire PRIVATE cxx_std_20)

add_custom_command(TARGET empire POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/assets"
		"$<TARGET_FILE_DIR:empire>/assets"
	COMMENT "Copying assets to build output"
)