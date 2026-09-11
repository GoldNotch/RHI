add_subdirectory (ThirdParty/VkBootstrap)

target_sources (${PROJECT_NAME}
PRIVATE
	"VulkanContext.cpp"
	"VulkanContext.hpp"
	"GarbageCollector.cpp"
	"GarbageCollector.hpp"
	"Device.cpp"
	"Device.hpp"
	"Surface.cpp"
	"Surface.hpp"

	"Memory/MemoryAllocator.hpp"
	"Memory/MemoryAllocator.cpp"
	"Memory/MemoryBlock.cpp"
	"Memory/MemoryBlock.hpp"
	"Memory/TextureInterface.hpp"
	"Memory/Texture.cpp"
	"Memory/Texture.hpp"
	"Memory/BufferGPU.cpp"
	"Memory/BufferGPU.hpp"
	"Memory/BufferInterface.hpp"
	"Memory/Synchronizer.cpp"
	"Memory/Synchronizer.hpp"
	"Memory/ResourceUser.hpp"
	
	"RenderPass/Framebuffer.cpp"
	"RenderPass/Framebuffer.hpp"
	"RenderPass/RenderPass.cpp"
	"RenderPass/RenderPass.hpp"
	"RenderPass/RenderTarget.cpp"
	"RenderPass/RenderTarget.hpp"
	"RenderPass/SubpassGraph.cpp"
	"RenderPass/SubpassGraph.hpp"

	"Pipeline/DescriptorBufferLayout.cpp"
	"Pipeline/DescriptorBufferLayout.hpp"
	"Pipeline/DescriptorsBuffer.cpp"
	"Pipeline/DescriptorsBuffer.hpp"
	"Pipeline/BaseDescriptor.hpp"
	"Pipeline/SamplerUniform.hpp"
	"Pipeline/SamplerUniform.cpp"
	"Pipeline/SamplerArrayUniform.hpp"
	"Pipeline/SamplerArrayUniform.cpp"
	"Pipeline/BufferUniform.cpp"
	"Pipeline/BufferUniform.hpp"
	"Pipeline/InputAttachmentUniform.cpp"
	"Pipeline/InputAttachmentUniform.hpp"
	"Pipeline/UpdateDescriptorTask.hpp"
	"Pipeline/Pipeline.cpp"
	"Pipeline/Pipeline.hpp"
	"Pipeline/PipelineProcess.hpp"
	"Pipeline/PipelineProcess.cpp"
	"Pipeline/PipelineAttachmentsUsage.cpp"
	"Pipeline/PipelineAttachmentsUsage.hpp"
	
	
	"Utils/CastHelper.hpp"
	"Utils/DescriptorSetLayoutBuilder.cpp"
	"Utils/DescriptorSetLayoutBuilder.hpp"
	"Utils/FramebufferBuilder.cpp"
	"Utils/FramebufferBuilder.hpp"
	"Utils/PipelineBuilder.cpp" 
	"Utils/PipelineBuilder.hpp"
	"Utils/PipelineLayoutBuilder.cpp"
	"Utils/PipelineLayoutBuilder.hpp"
	"Utils/SamplerBuilder.cpp"
	"Utils/SamplerBuilder.hpp"
	"Utils/FenceBuilder.hpp"
	"Utils/FenceBuilder.cpp"
	"Utils/SemaphoreBuilder.hpp"
	"Utils/SemaphoreBuilder.cpp"
	"Utils/EventBuilder.hpp"
	"Utils/EventBuilder.cpp"
	
	"ImageUtils/InternalImageTraits.hpp"
	"ImageUtils/ImageFormatsConversation.cpp"
	"ImageUtils/ImageFormatsConversation.hpp"
	"ImageUtils/ImageUtils.hpp"
	"ImageUtils/ImageUtils.cpp"

	"Attachments/GenericAttachment.cpp"
	"Attachments/GenericAttachment.hpp"
	"Attachments/Attachment.hpp"
	"Attachments/SurfacedAttachment.cpp"
	"Attachments/SurfacedAttachment.hpp"

	"TransferPass/Transferer.hpp"
	"TransferPass/Transferer.cpp"
	"TransferPass/TransferTask.hpp"
	"TransferPass/TransferTask.cpp"
	"TransferPass/TransferAlgorithm.hpp"
	"TransferPass/Blit.cpp"
	"TransferPass/Buffer.cpp"
	"TransferPass/Image.cpp"
	"TransferPass/Mipmaps.cpp"
	
	"CommandsExecution/InternalAwaitable.hpp"
	"CommandsExecution/CommandBuffer.cpp" 
	"CommandsExecution/CommandBuffer.hpp" 
	"CommandsExecution/Submitter.cpp"
	"CommandsExecution/Submitter.hpp"
	"CommandsExecution/SubmitTask.cpp"
	"CommandsExecution/SubmitTask.hpp"
	"CommandsExecution/DoubleBufferedSubmitter.hpp"
	"CommandsExecution/DoubleBufferedSubmitter.cpp"
)

#---------------- Link third-party libraries ---------------
#find_package(Vulkan REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(VulkanLoader REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

target_compile_definitions(${PROJECT_NAME} 
PRIVATE
	$<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)

target_link_libraries(${PROJECT_NAME}
PRIVATE
	#Vulkan::Vulkan
	vulkan-headers::vulkan-headers
	Vulkan::Loader
	vk-bootstrap::vk-bootstrap
	GPUOpen::VulkanMemoryAllocator
)

target_include_directories(${PROJECT_NAME}
PRIVATE
 "./"
)