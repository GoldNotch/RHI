#include "Texture.hpp"

#include <ImageUtils/ImageUtils.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
static constexpr uint32_t g_TextureUsageFlags =
  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

Texture::Texture(Context & ctx, const TextureDescription & args)
  : OwnedBy<Context>(ctx)
  , m_description(args)
  , m_memBlock(GetContext().GetBuffersAllocator().AllocImage(args, g_TextureUsageFlags,
                                                             VK_SAMPLE_COUNT_1_BIT))
  , m_synchronizer(ctx, m_memBlock.GetImage())
{
  m_view = utils::CreateImageView(GetContext().GetGpuConnection().GetDevice(),
                                  m_memBlock.GetImage(), GetInternalFormat(), GetImageViewType(),
                                  VK_IMAGE_ASPECT_COLOR_BIT);
}

Texture::~Texture()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(m_view), nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(m_memBlock), nullptr);
}

std::shared_ptr<IAwaitable> Texture::UploadImage(const UploadImageArgs & args)
{
  return GetContext().GetTransferer(QueueType::Graphics).UploadImage(*this, args);
}

std::shared_ptr<IAwaitable> Texture::DownloadImage(const DownloadImageArgs & args)
{
  return GetContext().GetTransferer(QueueType::Graphics).DownloadImage(*this, args);
}

std::shared_ptr<IAwaitable> Texture::GenerateMipmaps()
{
  return GetContext().GetTransferer(QueueType::Graphics).GenerateMipmaps(*this);
}

TextureDescription Texture::GetDescription() const noexcept
{
  return m_description;
}

size_t Texture::Size() const
{
  return m_memBlock.Size();
}

void Texture::BlitTo(ITexture * texture)
{
  if (auto * ptr = dynamic_cast<IInternalTexture *>(texture))
    GetContext()
      .GetTransferer(QueueType::Graphics)
      .BlitImageToImage(*ptr, *this, RHI::TextureRegion{});
}

VkImageView Texture::GetImageView() const noexcept
{
  return m_view;
}

VkImageLayout Texture::GetLayout() const noexcept
{
  return m_synchronizer.GetLayout();
}

VkImage Texture::GetHandle() const noexcept
{
  return m_memBlock.GetImage();
}

VkFormat Texture::GetInternalFormat() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkFormat>(m_description.format);
}

VkExtent3D Texture::GetInternalExtent() const noexcept
{
  auto [extent, _] = utils::UnpackExtentAndLayers(m_description.extent, m_description.type);
  return extent;
}

uint32_t Texture::GetMipLevelsCount() const noexcept
{
  return m_description.mipLevels;
}

uint32_t Texture::GetLayersCount() const noexcept
{
  auto [_, layersCount] = utils::UnpackExtentAndLayers(m_description.extent, m_description.type);
  return layersCount;
}

VkImageType Texture::GetImageType() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkImageType>(m_description.type);
}

VkImageViewType Texture::GetImageViewType() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkImageViewType>(m_description.type);
}

details::Synchronizer & Texture::GetSynchronizer() & noexcept
{
  return m_synchronizer;
}

} // namespace RHI::vulkan
