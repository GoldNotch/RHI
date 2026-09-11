#pragma once
#include <CommandsExecution/InternalAwaitable.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
struct SubmitTask : public IInternalAwaitable,
                    public OwnedBy<Context>
{
  explicit SubmitTask(Context & ctx);
  virtual ~SubmitTask() override;
  SubmitTask(SubmitTask && rhs) noexcept;
  SubmitTask & operator=(SubmitTask && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  RESTRICTED_COPY(SubmitTask);

public: // IAwaitable interface
  virtual bool Wait() noexcept override;
  virtual bool IsReady() const noexcept override;

public: // IInternalAwaitable
  virtual VkSemaphore GetSemaphore() const noexcept override { return m_semaphore; }
  virtual VkFence GetFence() const noexcept override { return m_fence; }

public:
  void StartTask() noexcept;

private:
  VkSemaphore m_semaphore = VK_NULL_HANDLE;
  VkFence m_fence = VK_NULL_HANDLE;
};

} // namespace RHI::vulkan
