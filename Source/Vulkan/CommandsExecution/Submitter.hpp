#pragma once

#include <CommandsExecution/SubmitTask.hpp>
#include <CommandsExecution/CommandBuffer.hpp>
#include <Device.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::details
{
/// @brief Submits commands into queue, owns primary command buffer
struct Submitter : public CommandBuffer
{
  explicit Submitter(Context & ctx, QueueType type, VkPipelineStageFlags waitStages);
  virtual ~Submitter() override;
  Submitter(Submitter && rhs) noexcept;
  Submitter & operator=(Submitter && rhs) noexcept;

  virtual SubmitTask * Submit(bool waitPrevSubmitOnGPU, std::span<const VkSemaphore> waitSemaphores);
  virtual void WaitForSubmitCompleted();

protected:
  VkPipelineStageFlags m_waitStages;
  QueueType m_queueType;

  SubmitTask m_oldBarrier;
  SubmitTask m_newBarrier;
  bool m_isFirstSubmit = true;
};

} // namespace RHI::vulkan::details
