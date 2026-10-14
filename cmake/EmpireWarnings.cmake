include_guard(GLOBAL)

add_library(empire_warnings INTERFACE)
add_library(empire::warnings ALIAS empire_warnings)

if(MSVC)
	target_compile_options(empire_warnings INTERFACE
		/W4
		/permissive-
		/Zc:preprocessor
		/utf-8
		"$<$<BOOL:${EMPIRE_WARNINGS_AS_ERRORS}>:/WX>"
	)
else()
	target_compile_options(empire_warnings INTERFACE
		-Wall
		-Wextra
		-Wpedantic
		"$<$<BOOL:${EMPIRE_WARNINGS_AS_ERRORS}>:-Werror>"
	)
endif()