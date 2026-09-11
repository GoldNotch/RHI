#include "RenderPass.hpp"

#include <CommandsExecution/Submitter.hpp>
#include <Memory/Synchronizer.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Pipeline/PipelineProcess.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <RenderPass/RenderTarget.hpp>
#include <RenderPass/SubpassGraph.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

RenderPass::RenderPass(Context & ctx, Framebuffer & framebuffer)
  : OwnedBy<Context>(ctx)
  , OwnedBy<Framebuffer>(framebuffer)
  , m_subpassGraph(new SubpassGraph())
  , m_execBuffer(ctx, ctx.GetGpuConnection().GetQueue(QueueType::Graphics).first,
                 VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_writeBuffer(ctx, ctx.GetGpuConnection().GetQueue(QueueType::Graphics).first,
                  VK_COMMAND_BUFFER_LEVEL_SECONDARY)
  , m_dummyPipeline(new Pipeline(ctx))
{
}

RenderPass::~RenderPass()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
}

void RenderPass::SetSubpass(uint32_t index, PipelinePtr pipeline, PipelineProcessPtr process)
{
  while (index >= m_subpasses.size())
    m_subpasses.push_back({nullptr, nullptr});
  // if pipeline has changed - we should rebuild renderPass
  // if process has changed - we should rewrite commands
  Subpass newSubpass = {FastDynamicCast<Pipeline>(pipeline),
                        FastDynamicCast<PipelineProcess>(process)};
  if (newSubpass.first != m_subpasses[index].first)
    m_invalidRenderPass = true;
  if (newSubpass.second != m_subpasses[index].second)
    m_dirtyCommands = true;
  m_subpasses[index] = newSubpass;
}

void RenderPass::ClearSubpasses()
{
  m_subpasses.clear();
  m_invalidRenderPass = true;
  m_dirtyCommands = true;
}

void RenderPass::RecordCommands(details::CommandBuffer & commands, RenderTarget & renderTarget)
{
  assert(m_renderPass);
  assert(renderTarget.GetAttachmentsCount() == m_subpassGraph->GetCachedAttachments().size());
  m_activeRenderTarget = &renderTarget;
  VkFramebuffer buf = renderTarget.GetHandle();
  VkExtent3D extent = renderTarget.GetVkExtent();
  auto && clearValues = renderTarget.GetClearValues();

  // here must be buffer synchronization
  for (auto [pipeline, process] : m_subpasses)
  {
    pipeline->SynchroniseResources(SynchronizationFilter::BufferOnly, commands);
    process->SynchroniseResources(SynchronizationFilter::BufferOnly, commands);
  }

  VkRenderPassBeginInfo renderPassInfo{};
  {
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = buf;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {extent.width, extent.height};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
  }
  commands
    .PushCommand(vkCmdBeginRenderPass, &renderPassInfo,
                 VK_SUBPASS_CONTENTS_INLINE); //TODO: VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
  m_subpassGraph->SynchronizeAttachmentsDuringRenderPass(SubpassIndex::initialRenderPass,
                                                         GetFramebuffer().GetAttachments());


  // execute commands for subpasses
  // RenderPass must have one subpass always!
  // if it's not, it'll be nothing to render
  if (!m_subpasses.empty())
  {
    for (size_t i = 0; auto && [pipeline, process] : m_subpasses)
    {
      // in renderPass you must not include any memoryBarrier,
      // so it's allowed ImageOnly synchronization
      pipeline->SynchroniseResources(SynchronizationFilter::ImageOnly, commands);
      process->SynchroniseResources(SynchronizationFilter::ImageOnly, commands);
      pipeline->BindToCommandBuffer(commands, VK_PIPELINE_BIND_POINT_GRAPHICS);
      process->RecordCommands(commands, *pipeline);

      m_subpassGraph->SynchronizeAttachmentsDuringRenderPass(static_cast<SubpassIndex>(i),
                                                             GetFramebuffer().GetAttachments());

      ++i;
      if (i < m_subpasses.size())
      {
        commands.PushCommand(
          vkCmdNextSubpass,
          VK_SUBPASS_CONTENTS_INLINE); //TODO: VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
      }
    }
  }
  else
  {
    m_dummyPipeline->BindToCommandBuffer(commands, VK_PIPELINE_BIND_POINT_GRAPHICS);
  }

  commands.PushCommand(vkCmdEndRenderPass);

  // probably it doesn't needed
  m_subpassGraph->SynchronizeAttachmentsDuringRenderPass(SubpassIndex::finalRenderPass,
                                                         GetFramebuffer().GetAttachments());
  m_activeRenderTarget = nullptr;
}

void RenderPass::CollectAttachmentsUsageInfo(std::span<VkImageUsageFlags> usage) const
{
  for (auto && [pipeline, _] : m_subpasses)
  {
    pipeline->GetAttachmentUsageInfo().CollectAttachmentsUsageInfo(usage);
  }
}

void RenderPass::CollectResources(std::vector<ResourcePtr> & resources) const
{
  for (auto && [pipeline, process] : m_subpasses)
  {
    pipeline->CollectResources(resources);
    // collect resources from draw commands (vertex/index buffers)
    process->CollectResources(resources);
  }
}

void RenderPass::SetAttachments(uint32_t buffersCount,
                                std::span<const VkAttachmentDescription> attachments) noexcept
{
  if (m_subpassGraph->SetAttachments(attachments))
    m_invalidRenderPass = true;
}

void RenderPass::Invalidate()
{
  bool rebuildSubpasses = false;
  if (m_invalidRenderPass || !m_renderPass)
  {
    std::vector<VkSubpassDescription> builtSubpasses;
    std::vector<SubpassIndex> selfDependencedSubpasses;
    builtSubpasses.reserve(m_subpasses.size());
    selfDependencedSubpasses.reserve(m_subpasses.size());
    for (uint32_t i = 0; auto && [pipeline, process] : m_subpasses)
    {
      builtSubpasses.push_back(
        pipeline->GetAttachmentUsageInfo().BuildDescription(VK_PIPELINE_BIND_POINT_GRAPHICS));
      if (pipeline->RequireSynchronization() || process->RequireSynchronization())
        selfDependencedSubpasses.push_back(static_cast<SubpassIndex>(i));
      ++i;
    }
    m_subpassGraph->BuildGraph(std::move(builtSubpasses), std::move(selfDependencedSubpasses));
    auto new_renderpass =
      m_subpassGraph->MakeRenderPass(GetContext().GetGpuConnection().GetDevice());
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkRenderPass({}) has been rebuilt - {}",
                     static_cast<void *>(m_renderPass), static_cast<void *>(new_renderpass));
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_renderPass, nullptr);
    m_renderPass = new_renderpass;
    m_invalidRenderPass = false;
    rebuildSubpasses = true;
  }

  //m_dummyPipeline->BuildAsGraphicPipeline(*this, 0);

  // rebuild pipelines
  for (uint32_t i = 0; auto && [pipeline, process] : m_subpasses)
  {
    pipeline->Invalidate(*this, i);
    //TODO: reset commands?
    ++i;
  }

  // rebuild commands
}

void RenderPass::SetInvalid()
{
  m_subpassGraph->ResetGraph();
  m_invalidRenderPass = true;
  m_dirtyCommands = true;
}
} // namespace RHI::vulkan
