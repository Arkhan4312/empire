include_guard(GLOBAL)

# empire_add_module(<name>
#	[STATIC | INTERFACE]
#	[SOURCES  <src>...]
#	[PUBLIC   <lib>...]
#	[PRIVATE  <lib>...]
#	[ALIAS    <empire::path::name>]
#)
#
# A thin wrapper over add_library that:
# - sets the correct library type,
# - propagates cxx_std_20 through PUBLIC,
# - links empire::warnings PRIVATE,
# - registers an ALIAS in the empire:: namespace.

function(empire_add_module name)
	cmake_parse_arguments(ARG
		"STATIC;INTERFACE"
		"ALIAS"
		"SOURCES;PUBLIC;PRIVATE"
		${ARGN})

	if(ARG_INTERFACE)
		add_library(${name} INTERFACE)
		if(ARG_PUBLIC)
			target_link_libraries(${name} INTERFACE ${ARG_PUBLIC})
		endif()
		if(ARG_PRIVATE)
			message(WARNING "empire_add_module(${name}): PRIVATE ignored for INTERFACE library")
		endif()


	elseif(ARG_STATIC)
		add_library(${name} STATIC ${ARG_SOURCES})
		if(ARG_PUBLIC)
			target_link_libraries(${name} PUBLIC ${ARG_PUBLIC})
		endif()
		if(ARG_PRIVATE)
			target_link_libraries(${name} PRIVATE ${ARG_PRIVATE})
		endif()
		target_compile_features(${name} PUBLIC cxx_std_20)
		target_link_libraries(${name} PRIVATE empire::warnings)


	else()
		message(FATAL_ERROR
				"empire_add_module(${name}): specify STATIC or INTERFACE")
	endif()

	if(ARG_ALIAS)
		add_library(${ARG_ALIAS} ALIAS ${name})
	endif()
endfunction()

	