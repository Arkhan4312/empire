add_library(empire_includes INTERFACE)
add_library(empire::includes ALIAS empire_includes)
target_include_directories(empire_includes INTERFACE
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
	"$<INSTALL_INTERFACE:include>")

add_subdirectory(empire/engine)
add_subdirectory(empire/game)

if(EMPIRE_BUILD_APP)
	add_subdirectory(app)
endif()