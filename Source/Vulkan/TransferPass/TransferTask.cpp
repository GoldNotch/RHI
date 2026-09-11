#include "TransferTask.hpp"

#include <Utils/SemaphoreBuilder.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
TransferTask::TransferTask(Context & ctx, RecordCommand && command, OnCompleteFunc && onComplete)
  : OwnedBy<Context>(ctx)
  //, m_semaphore(utils::SemaphoreBuilder().SetTimeline(0).Make(ctx.GetGpuConnection().GetDevice()))
  , m_command(std::move(command))
  , m_onComplete(std::move(onComplete))
{
  assert(m_command);
}

TransferTask::~TransferTask()
{
  //GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_semaphore, nullptr);
}

bool TransferTask::Wait() noexcept
{
  return m_submitTask ? m_submitTask->Wait() : false;
}

bool TransferTask::IsReady() const noexcept
{
  return m_submitTask ? m_submitTask->IsReady() : m_isReady.load();
}

VkSemaphore TransferTask::GetSemaphore() const noexcept
{
  return m_submitTask ? m_submitTask->GetSemaphore() : VK_NULL_HANDLE;
}

VkFence TransferTask::GetFence() const noexcept
{
  return m_submitTask ? m_submitTask->GetFence() : VK_NULL_HANDLE;
}

void TransferTask::RecordCommands(details::CommandBuffer & commands)
{
  if (m_command)
    m_command(commands);
}

void TransferTask::OnSubmit(SubmitTask & submitTask)
{
  m_submitTask = &submitTask;
}

void TransferTask::Complete()
{
  m_isReady = true;
  m_submitTask = nullptr;
  if (m_onComplete)
    m_onComplete(*this);
}

} // namespace RHI::vulkan
