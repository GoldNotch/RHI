#include "PipelineProcess.hpp"

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/BufferGPU.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Private/FastDynamicCast.hpp>
#include <Private/Overload.hpp>
#include <utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

PipelineProcess::PipelineProcess(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

PipelineProcess::~PipelineProcess()
{
}

void PipelineProcess::RecordCommands(details::CommandBuffer & commands, const Pipeline & pipeline)
{
  for (auto && task : m_commands)
  {
    if (task)
      task(commands, pipeline);
  }
}

void PipelineProcess::DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                                   std::uint32_t firstVertex, std::uint32_t firstInstance)
{
  auto task = [=](details::CommandBuffer & commands, const Pipeline & pipeline)
  {
    commands.PushCommand(vkCmdDraw, vertexCount, instanceCount, firstVertex, firstInstance);
  };
  m_commands.push_back(task);
}

void PipelineProcess::DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                          std::uint32_t firstIndex, int32_t vertexOffset,
                                          std::uint32_t firstInstance)
{
  auto task = [=](details::CommandBuffer & commands, const Pipeline & pipeline)
  {
    commands.PushCommand(vkCmdDrawIndexed, indexCount, instanceCount, firstIndex, vertexOffset,
                         firstInstance);
  };
  m_commands.push_back(task);
}

void PipelineProcess::SetViewport(float width, float height)
{
  auto task = [=](details::CommandBuffer & commands, const Pipeline & pipeline)
  {
    VkViewport vp{0.0f, 0.0f, width, height, 0.0f, 1.0f};
    commands.PushCommand(vkCmdSetViewport, 0, 1, &vp);
  };
  m_commands.push_back(task);
}

void PipelineProcess::SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height)
{
  auto task = [=](details::CommandBuffer & commands, const Pipeline & pipeline)
  {
    VkRect2D scissor{};
    scissor.extent = {width, height};
    scissor.offset = {x, y};
    commands.PushCommand(vkCmdSetScissor, 0, 1, &scissor);
  };
  m_commands.push_back(task);
}

void PipelineProcess::BindVertexBuffer(std::uint32_t binding, IBufferGPU * buffer,
                                       std::uint32_t offset)
{
  auto * internalBuffer = FastDynamicCast<IInternalBuffer>(buffer);
  if (!internalBuffer)
    return;
  auto task = [=](details::CommandBuffer & commands, const Pipeline & pipeline)
  {
    VkDeviceSize vkOffset = offset;
    VkBuffer buf = internalBuffer->GetHandle();
    commands.PushCommand(vkCmdBindVertexBuffers, binding, 1, &buf, &vkOffset);
  };
  m_resourceSyncInfos.push_back({internalBuffer, VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
                                 VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED});
  m_commands.push_back(task);
}

void PipelineProcess::BindIndexBuffer(IBufferGPU * buffer, IndexType type, std::uint32_t offset)
{
  auto * internalBuffer = FastDynamicCast<IInternalBuffer>(buffer);
  if (!internalBuffer)
    return;
  auto task = [=](details::CommandBuffer & commands, const Pipeline & pipeline)
  {
    commands.PushCommand(vkCmdBindIndexBuffer, internalBuffer->GetHandle(), VkDeviceSize{offset},
                         utils::CastInterfaceEnum2Vulkan<VkIndexType>(type));
  };

  m_resourceSyncInfos.push_back({internalBuffer, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
                                 VK_ACCESS_2_INDEX_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED});
  m_commands.push_back(task);
}

void PipelineProcess::PushConstant(const void * data, size_t size)
{
  std::vector<uint8_t> capturedData(size, 0);
  std::memcpy(capturedData.data(), data, size);
  auto task =
    [data = std::move(capturedData)](details::CommandBuffer & commands, const Pipeline & pipeline)
  {
    commands.PushCommand(vkCmdPushConstants, pipeline.GetPipelineLayoutHandle(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                         static_cast<uint32_t>(data.size()), data.data());
  };
  m_commands.push_back(task);
}

bool PipelineProcess::RequireSynchronization() const
{
  return !m_resourceSyncInfos.empty();
}

void PipelineProcess::CollectResources(std::vector<ResourcePtr> & resources) const
{
  for (auto && [ptr, _, __, ___] : m_resourceSyncInfos)
  {
    resources.push_back(ptr);
  }
}

void PipelineProcess::SynchroniseResources(SynchronizationFilter filter,
                                           details::CommandBuffer & commands) const
{
  for (auto && [objPtr, pipelineStage, access, layout] : m_resourceSyncInfos)
  {
    std::visit(std::overload(
                 [pipelineStage, access, layout, &commands, filter](IInternalBuffer * buffer)
                 {
                   if (buffer && FilterSatisfied(filter, SynchronizationFilter::BufferOnly))
                     buffer->GetSynchronizer().RequireSynchronize(pipelineStage, access, commands,
                                                                  layout);
                 },
                 [pipelineStage, access, layout, &commands, filter](IInternalTexture * texture)
                 {
                   if (texture && FilterSatisfied(filter, SynchronizationFilter::ImageOnly))
                     texture->GetSynchronizer().RequireSynchronize(pipelineStage, access, commands,
                                                                   layout);
                 }),
               objPtr);
  }
}
} // namespace RHI::vulkan
