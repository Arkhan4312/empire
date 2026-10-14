cmake_minimum_required(VERSION 3.24)

if(POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
		"$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,\
$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,\
$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(EmpireFromNothing VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSION OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(EmpireOptions)
include(EmpireWarnings)
include(EmpireDependencies)
include(EmpireHelpers)

if(EMPIRE_BUILD_APP)
	add_subdirectory(third_party)
endif()

add_subdirectory(src)

if(EMPIRE_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()