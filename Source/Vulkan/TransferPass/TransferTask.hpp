#pragma once
#include <functional>

#include <CommandsExecution/CommandBuffer.hpp>
#include <CommandsExecution/SubmitTask.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct TransferTask : public IInternalAwaitable,
                      public OwnedBy<Context>
{
  using RecordCommand = std::function<void(details::CommandBuffer &)>;
  using OnCompleteFunc = std::function<void(TransferTask &)>;

  explicit TransferTask(Context & ctx, RecordCommand && command, OnCompleteFunc && onComplete);
  virtual ~TransferTask() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  RESTRICTED_COPY(TransferTask);

public: // IAwaitable interface
  virtual bool Wait() noexcept override;
  virtual bool IsReady() const noexcept override;

public:
  virtual VkSemaphore GetSemaphore() const noexcept override;
  virtual VkFence GetFence() const noexcept override;

public: // internal interface
  void RecordCommands(details::CommandBuffer & commands);
  void OnSubmit(SubmitTask & submitTask);
  void Complete();

private:
  SubmitTask * m_submitTask = nullptr;
  std::atomic_bool m_isReady = false;
  RecordCommand m_command;
  OnCompleteFunc m_onComplete;
};
using TrasferTaskPtr = std::shared_ptr<TransferTask>;

} // namespace RHI::vulkan
