cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(subface LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
if(MSVC)
	add_definitions(-DCONSTEXPR=const -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
else()
	add_definitions(-DCONSTEXPR=constexpr)
	set(CXX_WARNINGS "-Wall -Wextra -Wpointer-arith -Wcast-align -fstrict-aliasing -Wno-unused-local-typedefs -Wno-misleading-indentation -Wno-maybe-uninitialized -Wno-int-in-bool-context -Wno-implicit-fallthrough")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_WARNINGS} -fvisibility-inlines-hidden -O3")

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
endif()

include_directories(
	src/core
	src/utility
	src/thirdparty
	src/thirdparty/glew-2.2.0/include
	src/thirdparty/glfw/include
	src/thirdparty/glm
	src/thirdparty/spdlog/include
	src/thirdparty/argparse/include
	${OPENGL_INCLUDE_DIR}
)

# Simplygon
if(WIN32)
	set(SIMPLYGON_10_PATH $ENV{SIMPLYGON_10_PATH} CACHE PATH "Path to Simplygon 10 SDK")
	if("${SIMPLYGON_10_PATH}" STREQUAL "")
		message(WARNING "SIMPLYGON_10_PATH is not set. Disabling decimation with Simplygon.")
	else()
		set(SIMPLYGON_10_LOADER "${SIMPLYGON_10_PATH}/SimplygonLoader.cpp")
		include_directories("${SIMPLYGON_10_PATH}")
		add_compile_definitions(USE_SIMPLYGON)
	endif()
else()
	message(WARNING "Simplygon only supports Windows. Disabling decimation with Simplygon.")
endif()

add_library(meshoptimizer
	src/thirdparty/meshoptimizer/simplifier.cpp
	src/thirdparty/meshoptimizer/vfetchoptimizer.cpp
)
add_library(core
	src/core/Subface.cpp
	src/utility/Timer.cpp
	${SIMPLYGON_10_LOADER}
)
target_link_libraries(core PRIVATE
	spdlog::spdlog
	meshoptimizer
)

add_library(utility
	src/utility/Camera.cpp
	src/utility/FPS.cpp
	src/utility/Model.cpp
	src/utility/OGL.cpp
	src/utility/Timer.cpp
	src/utility/Toggle.cpp
)
target_link_libraries(utility PRIVATE
	spdlog::spdlog
	glew_s
	glfw
)

add_executable(subface
	src/subface/subface.cpp
)
target_link_libraries(subface
	spdlog::spdlog
	core
	utility
)

if(MSVC)
	add_custom_target(copy_shader_files ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/src/shader
		${PROJECT_BINARY_DIR}/$<CONFIG>/shader
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/src/shader
		${PROJECT_BINARY_DIR}/shader
		COMMENT "Copying shader files"
	)
else()
	add_custom_target(copy_shader_files ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/src/shader
		${PROJECT_BINARY_DIR}/shader
		COMMENT "Copying shader files"
	)
endif()
add_dependencies(subface copy_shader_files)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/glew-2.2.0)
	file(DOWNLOAD
		https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip
		${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/glew-2.2.0.zip
		STATUS glew_download_status
	)
	list(GET glew_download_status 0 error_code)
	list(GET glew_download_status 1 error_reason)
	if(${error_code} EQUAL 0)
		file(ARCHIVE_EXTRACT
			INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/glew-2.2.0.zip"
			DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty"
		)
	else()
		message(FATAL_ERROR
			"Downloading glew-2.2.0.zip failed because of ${error_reason}!\n"
			"You may try configuring for another time.\n"
			"Or you can download it by yourself from \"https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip\" and extract to \"src/thirdparty\" so that \"src/thirdparty/glew-2.2.0/build/cmake\" exists."
		)
	endif()
endif()
option(BUILD_UTILS "utilities" OFF)
add_subdirectory(src/thirdparty/glew-2.2.0/build/cmake)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/glfw)
	message(FATAL_ERROR
		"\"src/thirdparty/glfw\" doesn't exist!\n"
		"Did you use \"--recursive\" when cloning?\n"
	)
endif()
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
add_subdirectory(src/thirdparty/glfw)

add_subdirectory(src/thirdparty/spdlog)
