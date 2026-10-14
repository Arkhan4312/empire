add_library(glad_gl_core_43 STATIC src/glad.c)
target_include_directories(glad_gl_core_43 PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")