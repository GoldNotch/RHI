project(Uniforms)

add_executable(${PROJECT_NAME} 
	"main.cpp"
)

target_link_libraries(${PROJECT_NAME} 
PRIVATE
	RHI
	glfw
)