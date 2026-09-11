#include <ImageUtils/ImageFormatsConversation.hpp>
#include <ImageUtils/ImageUtils.hpp>
#include <ImageUtils/InternalImageTraits.hpp>
#include <Memory/BufferGPU.hpp>
#include <TransferPass/TransferAlgorithm.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan::details
{

static constexpr uint32_t g_stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT;

TrasferTaskPtr UploadImage(Context & ctx, IInternalTexture & dstImage, const UploadImageArgs & args)
{
  const size_t copyingRegionSize =
    RHI::utils::GetSizeOfImage(args.copyRegion.extent, dstImage.GetInternalFormat());
  auto stagingBuffer = std::make_shared<BufferGPU>(ctx, copyingRegionSize, g_stagingUsage, true);

  auto [srcExtent, layersCount] =
    utils::UnpackExtentAndLayers(args.copyRegion.extent, args.srcTexture.type);

  auto [srcOffset, srcLayerBase] =
    utils::UnpackOffsetAndBaseLayer(args.copyRegion.offset, args.srcTexture.type);

  auto [dstOffset, dstLayerBase] =
    utils::UnpackOffsetAndBaseLayer(args.dstOffset, dstImage.GetImageType());

  if (auto && mapped_ptr = stagingBuffer->Map())
  {
    MappedGpuTextureView gpuTexture{};
    gpuTexture.pixelData = reinterpret_cast<uint8_t *>(mapped_ptr.get());
    gpuTexture.extent = args.copyRegion.extent;
    gpuTexture.format = dstImage.GetInternalFormat();
    gpuTexture.baseLayerIndex = srcLayerBase;
    gpuTexture.layersCount = layersCount;
    auto dstExtent = dstImage.GetInternalExtent();
    CopyImageFromHost(args.srcTexture, gpuTexture, args.copyRegion);
    mapped_ptr.reset();
    stagingBuffer->Flush();
  }
  else
  {
    throw std::runtime_error("Failed to fill staging buffer");
  }

  auto command = [stagingBuffer = std::move(stagingBuffer), &dstImage, srcExtent, dstOffset,
                  dstLayerBase, layersCount](details::CommandBuffer & commands)
  {
    VkBufferImageCopy region{};
    {
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;
      region.imageExtent = srcExtent;
      region.imageOffset = dstOffset;
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = dstLayerBase;
      region.imageSubresource.layerCount = layersCount;
    }

    VkImageLayout oldLayout = dstImage.GetLayout();
    dstImage.GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_COPY_BIT,
                                                  VK_ACCESS_2_TRANSFER_WRITE_BIT, commands,
                                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    commands.PushCommand(vkCmdCopyBufferToImage, stagingBuffer->GetHandle(), dstImage.GetHandle(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  };
  auto onComplete = [](TransferTask& task)
            {

            };

  return std::make_shared<TransferTask>(ctx, std::move(command), std::move(onComplete));
}

TrasferTaskPtr DownloadImage(Context & ctx, IInternalTexture & srcImage,
                             const DownloadImageArgs & args)
{
  auto stagingBuffer =
    std::make_shared<BufferGPU>(ctx,
                                RHI::utils::GetSizeOfImage(args.copyRegion.extent,
                                                           srcImage.GetInternalFormat()),
                                g_stagingUsage, true);

  auto [srcOffset, layerBase] =
    utils::UnpackOffsetAndBaseLayer(args.copyRegion.offset, srcImage.GetImageType());

  auto [srcExtent, layersCount] =
    utils::UnpackExtentAndLayers(args.copyRegion.extent, srcImage.GetImageType());


  auto command = [&srcImage, srcOffset, srcExtent, layerBase, layersCount,
                  stagingBuffer](details::CommandBuffer & commands)
  {
    VkBufferImageCopy region{};
    {
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;
      region.imageExtent = srcExtent;
      region.imageOffset = srcOffset;
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = layerBase;
      region.imageSubresource.layerCount = layersCount;
    }

    srcImage.GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_COPY_BIT,
                                                  VK_ACCESS_2_TRANSFER_READ_BIT, commands,
                                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    commands.PushCommand(vkCmdCopyImageToBuffer, srcImage.GetHandle(), srcImage.GetLayout(),
                         stagingBuffer->GetHandle(), 1, &region);
  };

  auto onComplete =
    [args, srcFormat = srcImage.GetInternalFormat(), stagingBuffer](TransferTask & task)
  {
    if (auto scopedPtr = stagingBuffer->Map())
    {
      MappedGpuTextureView view{};
      view.pixelData = reinterpret_cast<uint8_t *>(scopedPtr.get());
      view.extent = args.copyRegion.extent;
      view.format = srcFormat;
      CopyImageToHost(view, args.dstTexture, args.copyRegion);
    }
  };


  return std::make_shared<TransferTask>(ctx, std::move(command), std::move(onComplete));
}

} // namespace RHI::vulkan::details
