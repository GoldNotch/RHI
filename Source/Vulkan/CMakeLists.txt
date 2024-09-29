target_sources (${PROJECT_NAME}
PRIVATE
	"VulkanContext.cpp"
	"Swapchain.cpp"
	"Framebuffer.cpp" 
	"Pipeline.cpp" 
	"Utils/PipelineBuilder.cpp" 
	"Utils/FramebufferBuilder.cpp"
	"CommandBuffer.cpp" 
	"BufferGPU.cpp"
	"DescriptorsBuffer.cpp"
PRIVATE
	"VulkanContext.hpp"
	"Swapchain.hpp"
	"Framebuffer.hpp"
	"Pipeline.hpp"
	"Utils/Builders.hpp"
	"CommandBuffer.hpp"
	"BufferGPU.hpp"
	"DescriptorsBuffer.hpp"
)

#---------------- Link third-party libraries ---------------
find_package(Vulkan REQUIRED FATAL_ERROR)

message(STATUS "Fetch vkBootstrap")
FetchContent_Declare(
    vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        main #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)
FetchContent_MakeAvailable(vk_bootstrap)

message(STATUS "Fetch VulkanMemoryAllocator")
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        master #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
	GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)
#find_package(VulkanMemoryAllocator CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
PRIVATE
	Vulkan::Vulkan
	vk-bootstrap::vk-bootstrap
	GPUOpen::VulkanMemoryAllocator
)