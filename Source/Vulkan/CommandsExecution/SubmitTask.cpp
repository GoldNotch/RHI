#include "SubmitTask.hpp"

#include <Utils/FenceBuilder.hpp>
#include <Utils/SemaphoreBuilder.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

SubmitTask::SubmitTask(Context & ctx)
  : OwnedBy<Context>(ctx)
{
  m_semaphore = utils::SemaphoreBuilder().Make(ctx.GetGpuConnection().GetDevice());
  m_fence = utils::FenceBuilder().SetLocked().Make(ctx.GetGpuConnection().GetDevice());
}

SubmitTask::~SubmitTask()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_semaphore, nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_fence, nullptr);
}

SubmitTask::SubmitTask(SubmitTask && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::swap(m_fence, rhs.m_fence);
  std::swap(m_semaphore, rhs.m_semaphore);
}

SubmitTask & SubmitTask::operator=(SubmitTask && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_fence, rhs.m_fence);
    std::swap(m_semaphore, rhs.m_semaphore);
  }
  return *this;
}

bool SubmitTask::Wait() noexcept
{
  int res = VK_SUCCESS;
  if (m_fence)
  {
    res = vkWaitForFences(GetContext().GetGpuConnection().GetDevice(), 1, &m_fence, VK_TRUE,
                          UINT64_MAX);
  }
  return res == VK_SUCCESS;
}


bool SubmitTask::IsReady() const noexcept
{
  return vkGetFenceStatus(GetContext().GetGpuConnection().GetDevice(), m_fence) == VK_SUCCESS;
}


void SubmitTask::StartTask() noexcept
{
  Wait();
  vkResetFences(GetContext().GetGpuConnection().GetDevice(), 1, &m_fence);
}


} // namespace RHI::vulkan
