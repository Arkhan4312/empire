# CMakeList.txt: файл проекта CMake верхнего уровня; проведите глобальную конфигурацию
# и включите сюда подпроекты.
#
cmake_minimum_required (VERSION 3.20)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (EmpireFromNothing VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(EMPIRE_BUILD_TESTS "Build tests" ON)
option(EMPIRE_BUILD_APP "Build OpenGL application" ON)

include(FetchContent)

#nlohmann/json
FetchContent_Declare(nlohmann_json
	URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
	)
FetchContent_MakeAvailable(nlohmann_json)

if(EMPIRE_BUILD_APP)
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(glfw
	URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
	)
	FetchContent_MakeAvailable(glfw)

	FetchContent_Declare(glm
	URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz
	)
	FetchContent_MakeAvailable(glm)

	FetchContent_Declare(stb
	URL https://github.com/nothings/stb/archive/refs/heads/master.zip
	)
	FetchContent_MakeAvailable(stb)
	
	add_library(stb_headers INTERFACE)
	target_include_directories(stb_headers INTERFACE ${stb_SOURCE_DIR})

	add_library(glad_gl_core_43 STATIC "third_party/glad/src/glad.c")
	target_include_directories(glad_gl_core_43 PUBLIC "third_party/glad/include")
endif()

if(EMPIRE_BUILD_TESTS)
	FetchContent_Declare(Catch2
	URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
	)
	FetchContent_MakeAvailable(Catch2)
endif()

add_library(empire_core STATIC
	src/core/GameState.cpp
	src/core/GameLogic.cpp
	src/content/Content.cpp
	src/systems/ProductionSystem.cpp
	src/systems/ArmySystem.cpp
	src/systems/UpgradeSystem.cpp
	src/systems/EraSystem.cpp
	src/systems/SaveSystem.cpp
)

target_include_directories(empire_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(empire_core PUBLIC nlohmann_json::nlohmann_json)
target_compile_features(empire_core PUBLIC cxx_std_20)

if(MSVC)
	target_compile_options(empire_core PRIVATE /W4 /permissive-)
else()
	target_compile_options(empire_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EMPIRE_BUILD_APP)
	add_executable(empire
	src/main.cpp
	src/render/Window.cpp
	src/render/Shader.cpp
	src/render/Texture.cpp
	src/render/SpriteBatch.cpp
	src/render/Font.cpp
	src/render/Renderer.cpp
	)
	target_link_libraries(empire PRIVATE 
	empire_core
	glfw
	glad_gl_core_43
	glm::glm
	stb_headers
	)

	target_compile_definitions(empire PRIVATE 
	EMPIRE_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
	)

	add_custom_command(TARGET empire POST_BUILD 
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/assets"
			"$<TARGET_FILE_DIR:empire>/assets"
		COMMENT "Copying assets to build output"
	)
	
endif()

if(EMPIRE_BUILD_TESTS)
	enable_testing()
	add_executable(empire_tests
	tests/main_test.cpp
	tests/test_resources.cpp
	tests/test_production.cpp
	tests/test_army.cpp
	tests/test_upgrades.cpp
	tests/test_era.cpp
	tests/test_save.cpp
	tests/test_offline.cpp
	tests/test_balance.cpp
	tests/test_determinism.cpp
	)
	target_link_libraries(empire_tests PRIVATE empire_core Catch2::Catch2WithMain)

	include(CTest)
	include(Catch)
	catch_discover_tests(empire_tests)
endif()