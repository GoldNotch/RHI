
message(STATUS "Fetch GLFW3")
FetchContent_Declare(
    glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        master #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
	GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(glfw3)


add_custom_target("ExampleShaders" )

add_subdirectory(HelloTriangle)
add_subdirectory(Uniforms)