#include "DoubleBufferedSubmitter.hpp"

namespace RHI::vulkan
{
BufferedSubmitter::BufferedSubmitter(Context & ctx, QueueType type, uint32_t buffersCount,
                                     VkPipelineStageFlags waitStages)
  : details::Submitter(ctx, type, waitStages)
{
  m_submitters.reserve(buffersCount - 1);
  for (uint32_t i = 0; i < buffersCount - 1; ++i)
    m_submitters.emplace_back(ctx, type, waitStages);
  GetWritingBuffer().BeginWriting();
}

SubmitTask * BufferedSubmitter::Submit(bool waitPrevSubmitOnGPU,
                                      std::span<const VkSemaphore> waitSemaphores)
{
  if (m_submitters[m_writingSubmitterIdx].IsEmpty())
  {
    return nullptr;
  }
  m_submitters[m_writingSubmitterIdx].EndWriting();
  std::swap(m_submitters[m_writingSubmitterIdx], static_cast<details::Submitter &>(*this));
  SubmitTask * result = details::Submitter::Submit(waitPrevSubmitOnGPU, waitSemaphores);
  m_writingSubmitterIdx = (m_writingSubmitterIdx + 1) % m_submitters.size();
  m_submitters[m_writingSubmitterIdx].WaitForSubmitCompleted();
  m_submitters[m_writingSubmitterIdx].Reset();
  m_submitters[m_writingSubmitterIdx].BeginWriting();
  return result;
}
} // namespace RHI::vulkan
