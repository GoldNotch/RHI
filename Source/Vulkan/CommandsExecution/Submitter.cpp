#include "Submitter.hpp"

#include <VulkanContext.hpp>

namespace RHI::vulkan::details
{
Submitter::Submitter(Context & ctx, QueueType type, VkPipelineStageFlags waitStages)
  : CommandBuffer(ctx, ctx.GetGpuConnection().GetQueue(type).first, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
  , m_queueType(type)
  , m_waitStages(waitStages)
  , m_newBarrier(ctx)
  , m_oldBarrier(ctx)
{
}

Submitter::~Submitter()
{
  m_oldBarrier.Wait();
  m_newBarrier.Wait();
}

Submitter::Submitter(Submitter && rhs) noexcept
  : CommandBuffer(std::move(rhs))
  , m_newBarrier(std::move(rhs.m_newBarrier))
  , m_oldBarrier(std::move(rhs.m_oldBarrier))
{
  std::swap(m_waitStages, rhs.m_waitStages);
  std::swap(m_queueType, rhs.m_queueType);
  std::swap(m_isFirstSubmit, rhs.m_isFirstSubmit);
}

Submitter & Submitter::operator=(Submitter && rhs) noexcept
{
  if (this != &rhs)
  {
    CommandBuffer::operator=(std::move(rhs));
    std::swap(m_waitStages, rhs.m_waitStages);
    std::swap(m_newBarrier, rhs.m_newBarrier);
    std::swap(m_oldBarrier, rhs.m_oldBarrier);
    std::swap(m_queueType, rhs.m_queueType);
    std::swap(m_isFirstSubmit, rhs.m_isFirstSubmit);
  }
  return *this;
}

SubmitTask * Submitter::Submit(bool waitPrevSubmitOnGPU, std::span<const VkSemaphore> waitSemaphores)
{
  const VkSemaphore signalSem = m_newBarrier.GetSemaphore();
  const VkCommandBuffer buffer = GetHandle();
  auto [_, queue] = GetContext().GetGpuConnection().GetQueue(m_queueType);

  //if (!m_isFirstSubmit && waitPrevSubmitOnGPU)
  //  waitSemaphores.push_back(m_oldBarrier.GetSemaphore());

  assert(std::all_of(waitSemaphores.begin(), waitSemaphores.end(),
                     [](VkSemaphore sem) { return !!sem; }));

  std::vector<VkPipelineStageFlags> waitStages(waitSemaphores.size(), m_waitStages);
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  submitInfo.pWaitSemaphores = waitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &buffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &signalSem;

  m_newBarrier.StartTask();
  auto res = vkQueueSubmit(queue, 1, &submitInfo, m_newBarrier.GetFence());
  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to submit command buffer!");
  std::swap(m_oldBarrier, m_newBarrier);
  m_isFirstSubmit = false;
  return &m_oldBarrier;
}

void Submitter::WaitForSubmitCompleted()
{
  m_oldBarrier.Wait();
}

} // namespace RHI::vulkan::details
