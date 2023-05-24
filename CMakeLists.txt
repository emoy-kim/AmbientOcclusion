cmake_minimum_required(VERSION 3.8)

project("AmbientOcclusion")

include(cmake/check-compiler.cmake)

set(CMAKE_CXX_STANDARD 17)

set(
	SOURCE_FILES 
		main.cpp
		source/text.cpp
		source/light.cpp
		source/camera.cpp
		source/object.cpp
		source/shader.cpp
		source/renderer.cpp
		source/occlusion_tree.cpp
	  	source/surface_element.cpp
)

configure_file(include/project_constants.h.in ${PROJECT_BINARY_DIR}/project_constants.h @ONLY)

include_directories("include")
include(cmake/add-libraries-linux.cmake)

add_executable(AmbientOcclusion ${SOURCE_FILES})

include(cmake/target-link-libraries-linux.cmake)

target_include_directories(AmbientOcclusion PUBLIC ${CMAKE_BINARY_DIR})