#include <TransferPass/TransferAlgorithm.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan::details
{

TrasferTaskPtr BlitImageToImage(Context & ctx, IInternalTexture & dst, IInternalTexture & src,
                                const TextureRegion & region)
{
  auto command = [&src, &dst](details::CommandBuffer & commands)
  {
    VkImageCopy copy{};
    {
      //TODO: Fill copy
      copy.extent = src.GetInternalExtent();
      //copy.dstOffset =
    }
    commands.PushCommand(vkCmdCopyImage, src.GetHandle(), src.GetLayout(), dst.GetHandle(),
                         dst.GetLayout(), 1, &copy);
  };

  return std::make_shared<TransferTask>(ctx, std::move(command), nullptr);
}

} // namespace RHI::vulkan::details
