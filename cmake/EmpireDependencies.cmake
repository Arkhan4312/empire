include_guard(GLOBAL)
include(FetchContent)

# nlohmann_json
# game::content, game::save, engine::settings

if(EMPIRE_BUILD_APP OR EMPIRE_BUILD_TESTS)
	fetchContent_Declare(nlohmann_json
		URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
	FetchContent_MakeAvailable(nlohmann_json)
endif()

# App-only dependencies
if(EMPIRE_BUILD_APP)
	set(GLFW_BUILD_DOCS 	OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS 	OFF CACHE BOOL "" FORCE)
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
	
	FetchContent_Declare(miniaudio
	URL https://github.com/mackron/miniaudio/archive/refs/heads/master.zip
	)
	FetchContent_MakeAvailable(miniaudio)

	add_library(miniaudio_headers INTERFACE)
	target_include_directories(miniaudio_headers INTERFACE ${miniaudio_SOURCE_DIR})

	add_library(stb_headers INTERFACE)
	target_include_directories(stb_headers INTERFACE ${stb_SOURCE_DIR})
endif()

# tests only
if(EMPIRE_BUILD_TESTS)
	FetchContent_Declare(Catch2
		URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz)
	FetchContent_MakeAvailable(Catch2)
endif()
