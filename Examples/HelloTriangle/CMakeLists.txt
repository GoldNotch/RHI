project(HelloTriangle)

add_executable(${PROJECT_NAME} 
	"main.cpp"
)

target_link_libraries(${PROJECT_NAME} 
PRIVATE
	RHI
	glfw
)

TARGET_PRECOMPILE_SHADERS(TARGET ${PROJECT_NAME} API Vulkan
SHADERS
	"triangle.frag"
	"triangle.vert"
)