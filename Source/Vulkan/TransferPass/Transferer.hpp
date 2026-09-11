#pragma once
#include <functional>
#include <list>
#include <mutex>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <CommandsExecution/SubmitTask.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct TransferTask;
} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct Transferer final : public OwnedBy<Context>,
                          public IResourceUser,
                          public ICommandWriter
{
  explicit Transferer(Context & ctx, uint32_t queueFamily, uint32_t buffersCount);
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // ICommandWriter
  virtual void RecordCommands(details::CommandBuffer & commands) override;

public: // IResourceUser
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const override;
  virtual void SynchroniseResources(SynchronizationFilter filter, details::CommandBuffer & commands) const override;

public:
  void OnSubmit(SubmitTask & submitTask);
  void ProcessExecutingCommands();

public:
  std::shared_ptr<IAwaitable> UploadBuffer(IInternalBuffer & dstBuffer, const uint8_t * srcData,
                                           size_t size, size_t offset = 0);
  std::shared_ptr<IAwaitable> DownloadBuffer(IInternalBuffer & srcBuffer, size_t size,
                                             size_t offset = 0);

  std::shared_ptr<IAwaitable> UploadImage(IInternalTexture & dstImage,
                                          const UploadImageArgs & args);
  std::shared_ptr<IAwaitable> DownloadImage(IInternalTexture & srcImage,
                                            const DownloadImageArgs & args);
  std::shared_ptr<IAwaitable> BlitImageToImage(IInternalTexture & dst, IInternalTexture & src,
                                               const TextureRegion & region);
  std::shared_ptr<IAwaitable> GenerateMipmaps(IInternalTexture & texture);

private:
  using TasksBatch = std::vector<std::shared_ptr<TransferTask>>;
  mutable std::mutex m_writeLock;
  std::mutex m_execLock;
  TasksBatch m_writingTasks;
  std::list<TasksBatch> m_executingTasks;
  std::vector<ResourcePtr> m_resourcesToSync;

private:
  void WriteNewTask(std::shared_ptr<TransferTask> task,
                    std::initializer_list<ResourcePtr> resourcesToSync);
};

} // namespace RHI::vulkan
