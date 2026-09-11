#pragma once
#include <CommandsExecution/Submitter.hpp>

namespace RHI::vulkan
{

struct BufferedSubmitter final : public details::Submitter
{
  explicit BufferedSubmitter(Context & ctx, QueueType type, uint32_t buffersCount,
                             VkPipelineStageFlags waitStages);

  details::Submitter & GetWritingBuffer() & noexcept { return m_submitters[m_writingSubmitterIdx]; }
  virtual SubmitTask * Submit(bool waitPrevSubmitOnGPU,
                             std::span<const VkSemaphore> waitSemaphores) override;

private:
  std::vector<details::Submitter> m_submitters;
  uint32_t m_writingSubmitterIdx = 0;
};
} // namespace RHI::vulkan
