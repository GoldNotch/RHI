set(this_target "test_utils")

add_library(${this_target})

target_sources (${this_target}
PUBLIC
	"Camera.hpp"
	"Window.hpp"
	"TestUtils.hpp"
	"test_utils_def.h"
PRIVATE
	"Camera.cpp"
	"Window.cpp"
	"TestUtils.cpp"
)

target_include_directories(${this_target} PUBLIC "./")

find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(stb CONFIG REQUIRED)
find_package(rectpack2d CONFIG REQUIRED)

target_link_libraries(${this_target} 
PRIVATE
	glm::glm
	stb::stb
	glfw
	RHI
	rectpack2d::rectpack2d
)

target_include_directories(${this_target}
PRIVATE
	"../stb"
)

target_compile_definitions(${this_target}
PRIVATE
	TEST_UTILS_BUILD
)