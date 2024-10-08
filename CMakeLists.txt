cmake_minimum_required(VERSION 3.14)
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
include(Source/ShaderCompile.cmake)

option(RHI_BUILD_EXAMPLES "Build example programs" ON)
option(RHI_VULKAN_BACKEND "use vulkan as backend" ON)

project(RHI C CXX)
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/Source)
set(APP_DIR ${PROJECT_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE})
set(EXAMPLES_DIR ${PROJECT_SOURCE_DIR}/Examples)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${APP_DIR}/Obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${APP_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${APP_DIR})
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_C_STANDATD 17)
cmake_policy(SET CMP0135 NEW)
if(LINUX)
add_compile_options(-fPIC)
endif()




ADD_SUBDIRECTORY(${SOURCE_DIR})
if (${RHI_BUILD_EXAMPLES})
	add_subdirectory(${EXAMPLES_DIR})
endif(${RHI_BUILD_EXAMPLES})