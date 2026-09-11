#include "Transferer.hpp"

#include <TransferPass/TransferAlgorithm.hpp>
#include <TransferPass/TransferTask.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

Transferer::Transferer(Context & ctx, uint32_t queueFamily, uint32_t buffersCount)
  : OwnedBy<Context>(ctx)
{
}

void Transferer::RecordCommands(details::CommandBuffer & commands)
{
  std::lock_guard lk{m_writeLock};
  if (m_writingTasks.empty())
    return;
  for (auto && taskPtr : m_writingTasks)
    taskPtr->RecordCommands(commands);
  m_resourcesToSync.clear();
  std::lock_guard lk2{m_execLock};
  m_executingTasks.push_front(std::move(m_writingTasks));
}

void Transferer::CollectResources(std::vector<ResourcePtr> & resources) const
{
  std::lock_guard lk{m_writeLock};
  resources.insert(resources.end(), m_resourcesToSync.begin(), m_resourcesToSync.end());
}

void Transferer::SynchroniseResources(SynchronizationFilter filter, details::CommandBuffer & commands) const
{
  //Do nothing, because commands synchronise resources themselfs
}

void Transferer::OnSubmit(SubmitTask & submitTask)
{
  std::lock_guard lk{m_execLock};
  if (m_executingTasks.empty())
    return;
  auto && batch = m_executingTasks.front();
  for (auto && task : batch)
  {
    task->OnSubmit(submitTask);
  }
}

void Transferer::ProcessExecutingCommands()
{
  std::lock_guard lk{m_execLock};
  for (auto it = m_executingTasks.begin(); it != m_executingTasks.end();)
  {
    auto & batch = *it;

    // Complete all ready tasks first
    for (auto & task : batch)
    {
      if (task->IsReady())
        task->Complete();
    }

    // Remove all completed tasks
    auto newEnd =
      std::remove_if(batch.begin(), batch.end(), [](const auto & task) { return task->IsReady(); });
    batch.erase(newEnd, batch.end());

    if (batch.empty())
      it = m_executingTasks.erase(it);
    else
      ++it;
  }
}

std::shared_ptr<IAwaitable> Transferer::UploadBuffer(IInternalBuffer & dstBuffer,
                                                     const uint8_t * srcData, size_t size,
                                                     size_t offset)
{
  auto task = details::UploadBuffer(GetContext(), dstBuffer, srcData, size, offset);
  if (!task)
    return nullptr;
  WriteNewTask(task, {&dstBuffer});
  return task;
}

std::shared_ptr<IAwaitable> Transferer::DownloadBuffer(IInternalBuffer & srcBuffer, size_t size,
                                                       size_t offset)
{
  auto task = details::DownloadBuffer(GetContext(), srcBuffer, nullptr, size, offset);
  if (!task)
    return nullptr;
  WriteNewTask(task, {&srcBuffer});
  return task;
}

std::shared_ptr<IAwaitable> Transferer::UploadImage(IInternalTexture & dstImage,
                                                    const UploadImageArgs & args)
{
  auto task = details::UploadImage(GetContext(), dstImage, args);
  if (!task)
    return nullptr;
  WriteNewTask(task, {&dstImage});
  return task;
}

std::shared_ptr<IAwaitable> Transferer::DownloadImage(IInternalTexture & srcImage,
                                                      const DownloadImageArgs & args)
{
  auto task = details::DownloadImage(GetContext(), srcImage, args);
  if (!task)
    return nullptr;
  WriteNewTask(task, {&srcImage});
  return task;
}

std::shared_ptr<IAwaitable> Transferer::BlitImageToImage(IInternalTexture & dst,
                                                         IInternalTexture & src,
                                                         const TextureRegion & region)
{
  auto task = details::BlitImageToImage(GetContext(), dst, src, region);
  if (!task)
    return nullptr;
  WriteNewTask(task, {&dst, &src});
  return task;
}

std::shared_ptr<IAwaitable> Transferer::GenerateMipmaps(IInternalTexture & texture)
{
  auto task = details::GenerateMipmaps(GetContext(), texture);
  if (!task)
    return nullptr;
  WriteNewTask(task, {&texture});
  return task;
}

void Transferer::WriteNewTask(TrasferTaskPtr task,
                              std::initializer_list<ResourcePtr> resourcesToSync)
{
  std::lock_guard lk{m_writeLock};
  m_writingTasks.push_back(std::move(task));
  m_resourcesToSync.insert(m_resourcesToSync.end(), resourcesToSync.begin(), resourcesToSync.end());
}

} // namespace RHI::vulkan
