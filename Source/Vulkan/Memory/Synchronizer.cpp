#include "Synchronizer.hpp"

#include <CommandsExecution/CommandBuffer.hpp>
#include <VulkanContext.hpp>

namespace
{
VkPipelineStageFlags ConvertPipelineStageFromVk2ToVk1(VkPipelineStageFlags2 flags) noexcept
{
  // clang-format off
  static const std::unordered_map<VkPipelineStageFlags2, VkPipelineStageFlags> m = {
    // Graphics pipeline stages
    {VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT},
    {VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT},
    {VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
    {VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
    {VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
    {VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
    {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    {VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
    {VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
    // Compute pipeline
    {VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},

    // Transfer stages - multiple VK2 flags map to VK_PIPELINE_STAGE_TRANSFER_BIT
    {VK_PIPELINE_STAGE_2_COPY_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_RESOLVE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_BLIT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},

    // Host and bottom
    {VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT},
    {VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},

    // Combined flags
    {VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT},

    // Ray tracing (if supported)
    {VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR},
    {VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR},

    // Task/Mesh shaders (Vulkan 1.3)
    {VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT, VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT},
    {VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT},

    // Fragment density map
    {VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT, VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT},
  };
  // clang-format on

  auto it = m.find(flags);
  if (it != m.end())
    return it->second;
  return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
}

VkAccessFlags ConvertAccessFlagsFromVk2ToVk1(VkAccessFlags2 flags) noexcept
{
  // clang-format off
  static const std::unordered_map<VkAccessFlags2, VkAccessFlags> m =
  {
     {VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT},
     {VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
     {VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT},
     {VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
     {VK_ACCESS_2_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT},
     {VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT},
     {VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT},
     {VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT},
     {VK_ACCESS_2_HOST_READ_BIT, VK_ACCESS_HOST_READ_BIT},
     {VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_HOST_WRITE_BIT},
     {VK_ACCESS_2_MEMORY_READ_BIT, VK_ACCESS_MEMORY_READ_BIT},
     {VK_ACCESS_2_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT},
     {VK_ACCESS_2_INDEX_READ_BIT, VK_ACCESS_INDEX_READ_BIT},
     {VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT},
     {VK_ACCESS_2_UNIFORM_READ_BIT, VK_ACCESS_UNIFORM_READ_BIT},
     {VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT},
     {VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV, VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV},
     {VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV, VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV},
     {VK_ACCESS_2_NONE, VK_ACCESS_NONE},

     // Extended flags
     {VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV, VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV },
     {VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV, VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV},
     {VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT, VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT},
     {VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT, VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT},
     {VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR},
     {VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR},
     //{VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR, VK_ACCESS_SHADER_BINDING_TABLE_READ_BIT_KHR},
     {VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT, VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT},
     {VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT, VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT},
     {VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT, VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT},
  };
  // clang-format on

  auto it = m.find(flags);
  if (it != m.end())
    return it->second;
  return VK_ACCESS_NONE;
}

bool IsFramebufferSpaceStage(VkPipelineStageFlags stage) noexcept
{
  const VkPipelineStageFlags framebufferStages =
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  return static_cast<bool>(stage & framebufferStages);
}
} // namespace

namespace RHI::vulkan::details
{

Synchronizer::Synchronizer(Context & ctx, VkImage image)
  : OwnedBy<Context>(ctx)
  , m_image(image)
{
  std::lock_guard lk{m_syncMutex};
  m_prevBarrier = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};
}

Synchronizer::Synchronizer(Context & ctx, VkBuffer buffer)
  : OwnedBy<Context>(ctx)
  , m_buffer(buffer)
{
  std::lock_guard lk{m_syncMutex};
  m_prevBarrier = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};
}

Synchronizer::Synchronizer(Synchronizer && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::lock_guard lk1(m_syncMutex);
  std::lock_guard lk2(rhs.m_syncMutex);
  std::swap(m_image, rhs.m_image);
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_prevBarrier, rhs.m_prevBarrier);
}

Synchronizer & Synchronizer::operator=(Synchronizer && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::lock_guard lk1(m_syncMutex);
    std::lock_guard lk2(rhs.m_syncMutex);
    std::swap(m_image, rhs.m_image);
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_prevBarrier, rhs.m_prevBarrier);
  }
  return *this;
}

Synchronizer::~Synchronizer()
{
}

void Synchronizer::ResetSynchronization()
{
  std::lock_guard lk{m_syncMutex};
  m_prevBarrier = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                   // don't reset layout for images
                   m_buffer ? VK_IMAGE_LAYOUT_UNDEFINED : m_prevBarrier.requiredLayout};
}

void Synchronizer::RequireSynchronize(VkPipelineStageFlags2 currentStage,
                                      VkAccessFlagBits2 requiredAccess,
                                      details::CommandBuffer & commands,
                                      VkImageLayout requiredLayout /* = VK_IMAGE_LAYOUT_UNDEFINED*/)
{
  assert(!m_image || requiredLayout != VK_IMAGE_LAYOUT_UNDEFINED);
  BarrierInfo barrierInfo{};
  barrierInfo.currentStage = currentStage;
  barrierInfo.requiredAccess = requiredAccess;
  barrierInfo.requiredLayout = requiredLayout;
  VkDependencyFlags dependencyFlags = 0;
  if (IsFramebufferSpaceStage(m_prevBarrier.currentStage))
    dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  std::lock_guard lk{m_syncMutex};
  if (GetContext().GetGpuConnection().CheckExtension("VK_KHR_synchronization2") &&
      GetContext().GetGpuConnection().GetVulkanVersion() >= VK_API_VERSION_1_3)
  {
    VkDependencyInfo info{};
    VkImageMemoryBarrier2 imageBarrier{};
    VkBufferMemoryBarrier2 bufferBarrier{};
    info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    info.dependencyFlags = dependencyFlags;
    if (m_image)
    {
      imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
      imageBarrier.pNext = nullptr;
      imageBarrier.image = m_image;
      imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageBarrier.subresourceRange.baseMipLevel = 0;
      imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
      imageBarrier.subresourceRange.baseArrayLayer = 0;
      imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
      imageBarrier.dstStageMask = currentStage;
      imageBarrier.dstAccessMask = requiredAccess;
      imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.srcStageMask = m_prevBarrier.currentStage;
      imageBarrier.srcAccessMask = m_prevBarrier.requiredAccess;
      imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.newLayout = requiredLayout;
      imageBarrier.oldLayout = m_prevBarrier.requiredLayout;
      info.imageMemoryBarrierCount = 1;
      info.pImageMemoryBarriers = &imageBarrier;
    }
    else if (m_buffer)
    {
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
      bufferBarrier.pNext = nullptr;
      bufferBarrier.buffer = m_buffer;
      bufferBarrier.offset = 0;
      bufferBarrier.size = std::numeric_limits<VkDeviceSize>::max();
      bufferBarrier.dstStageMask = currentStage;
      bufferBarrier.dstAccessMask = requiredAccess;
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.srcStageMask = m_prevBarrier.currentStage;
      bufferBarrier.srcAccessMask = m_prevBarrier.requiredAccess;
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      info.bufferMemoryBarrierCount = 1;
      info.pBufferMemoryBarriers = &bufferBarrier;
    }
    else
    {
      throw std::runtime_error("Unknown object for barrier");
    }

    commands.PushCommand(vkCmdPipelineBarrier2, &info);
  }
  else
  {
    VkImageMemoryBarrier imageBarrier{};
    VkBufferMemoryBarrier bufferBarrier{};
    if (m_image)
    {
      imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageBarrier.pNext = nullptr;
      imageBarrier.image = m_image;
      imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageBarrier.subresourceRange.baseMipLevel = 0;
      imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
      imageBarrier.subresourceRange.baseArrayLayer = 0;
      imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
      imageBarrier.dstAccessMask = ConvertAccessFlagsFromVk2ToVk1(requiredAccess);
      imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.srcAccessMask = ConvertAccessFlagsFromVk2ToVk1(m_prevBarrier.requiredAccess);
      imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.newLayout = requiredLayout;
      imageBarrier.oldLayout = m_prevBarrier.requiredLayout;
    }
    else if (m_buffer)
    {
      bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      bufferBarrier.pNext = nullptr;
      bufferBarrier.buffer = m_buffer;
      bufferBarrier.offset = 0;
      bufferBarrier.size = std::numeric_limits<VkDeviceSize>::max();
      bufferBarrier.dstAccessMask = ConvertAccessFlagsFromVk2ToVk1(requiredAccess);
      bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufferBarrier.srcAccessMask = ConvertAccessFlagsFromVk2ToVk1(m_prevBarrier.requiredAccess);
      bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }
    else
    {
      throw std::runtime_error("Unknown object for barrier");
    }
    commands.PushCommand(vkCmdPipelineBarrier,
                         ConvertPipelineStageFromVk2ToVk1(m_prevBarrier.currentStage),
                         ConvertPipelineStageFromVk2ToVk1(currentStage), dependencyFlags, 0,
                         nullptr, m_buffer ? 1 : 0, &bufferBarrier, m_image ? 1 : 0, &imageBarrier);
  }
  m_prevBarrier = barrierInfo;
}


VkImageLayout Synchronizer::GetLayout() const noexcept
{
  std::lock_guard lk{m_syncMutex};
  return m_prevBarrier.requiredLayout;
}

void Synchronizer::ExternalSynchronization(const BarrierInfo & barrier) noexcept
{
  std::lock_guard lk{m_syncMutex};
  m_prevBarrier = barrier;
}

} // namespace RHI::vulkan::details
