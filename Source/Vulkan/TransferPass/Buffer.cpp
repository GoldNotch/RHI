#include <Memory/BufferGPU.hpp>
#include <TransferPass/TransferAlgorithm.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan::details
{

static constexpr uint32_t g_stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT;

TrasferTaskPtr UploadBuffer(Context & ctx, IInternalBuffer & dstBuffer, const uint8_t * srcData,
                            size_t size, size_t offset)
{
  auto stagingBuffer = std::make_shared<BufferGPU>(ctx, size - offset, g_stagingUsage, true);
  stagingBuffer->UploadSync(srcData, size, offset);

  auto command =
    [stagingBuffer = std::move(stagingBuffer), &dstBuffer](details::CommandBuffer & commands)
  {
    dstBuffer.GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_COPY_BIT,
                                                   VK_ACCESS_2_TRANSFER_WRITE_BIT, commands);
    VkBufferCopy copy{};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = stagingBuffer->GetSize();
    commands.PushCommand(vkCmdCopyBuffer, stagingBuffer->GetHandle(), dstBuffer.GetHandle(), 1,
                         &copy);
  };

  auto onComplete = [](TransferTask& task) {};


  return std::make_shared<TransferTask>(ctx, std::move(command), std::move(onComplete));
}

TrasferTaskPtr DownloadBuffer(Context & ctx, IInternalBuffer & srcBuffer, uint8_t * dst,
                              size_t size, size_t offset)
{
  auto stagingBuffer = std::make_shared<BufferGPU>(ctx, size - offset, g_stagingUsage, true);

  auto command = [&srcBuffer, size, offset, stagingBuffer](details::CommandBuffer & commands)
  {
    srcBuffer.GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_COPY_BIT,
                                                   VK_ACCESS_2_TRANSFER_READ_BIT, commands);
    VkBufferCopy copy{};
    copy.dstOffset = 0;
    copy.srcOffset = offset;
    copy.size = size;
    commands.PushCommand(vkCmdCopyBuffer, srcBuffer.GetHandle(), stagingBuffer->GetHandle(), 1,
                         &copy);
  };

  auto onComplete = [dst, stagingBuffer](TransferTask & task)
  {
    if (auto scopedPtr = stagingBuffer->Map())
      std::memcpy(dst, scopedPtr.get(), stagingBuffer->Size());
  };


  return std::make_shared<TransferTask>(ctx, std::move(command), std::move(onComplete));
}

} // namespace RHI::vulkan::details
