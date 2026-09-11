#include "GenericAttachment.hpp"

#include <ImageUtils/ImageUtils.hpp>
#include <ImageUtils/InternalImageTraits.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace
{

constexpr VkImageLayout MakeAttachmentInitialLayout(RHI::ImageFormat format)
{
  return VK_IMAGE_LAYOUT_UNDEFINED;
}

constexpr VkImageLayout MakeAttachmentFinalLayout(RHI::ImageFormat format)
{
  switch (format)
  {
    case RHI::ImageFormat::A8:
    case RHI::ImageFormat::R8:
    case RHI::ImageFormat::RG8:
    case RHI::ImageFormat::RGB8:
    case RHI::ImageFormat::RGBA8:
    case RHI::ImageFormat::BGR8:
    case RHI::ImageFormat::BGRA8:
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RHI::ImageFormat::DEPTH:
      return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    case RHI::ImageFormat::DEPTH_STENCIL:
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    default:
      return VK_IMAGE_LAYOUT_UNDEFINED;
  }
}

constexpr VkImageAspectFlags CalcImageAspectByFormat(RHI::ImageFormat format)
{
  switch (format)
  {
    case RHI::ImageFormat::A8:
    case RHI::ImageFormat::R8:
    case RHI::ImageFormat::RG8:
    case RHI::ImageFormat::RGB8:
    case RHI::ImageFormat::RGBA8:
    case RHI::ImageFormat::BGR8:
    case RHI::ImageFormat::BGRA8:
      return VK_IMAGE_ASPECT_COLOR_BIT;
    case RHI::ImageFormat::DEPTH:
      return VK_IMAGE_ASPECT_DEPTH_BIT;
    case RHI::ImageFormat::DEPTH_STENCIL:
      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default:
      return VK_IMAGE_ASPECT_NONE;
  }
}

VkAttachmentDescription BuildAttachmentDescription(const RHI::TextureDescription & description,
                                                   RHI::SamplesCount samplesCount) noexcept
{
  VkAttachmentDescription attachmentDescription{};
  {
    attachmentDescription.format =
      RHI::vulkan::utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
    attachmentDescription.samples =
      RHI::vulkan::utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(samplesCount);
    attachmentDescription.initialLayout = MakeAttachmentInitialLayout(description.format);
    attachmentDescription.finalLayout = MakeAttachmentFinalLayout(description.format);
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = samplesCount == RHI::SamplesCount::One
                                    ? VK_ATTACHMENT_STORE_OP_STORE
                                    : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.stencilStoreOp = samplesCount == RHI::SamplesCount::One
                                           ? VK_ATTACHMENT_STORE_OP_STORE
                                           : VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }
  return attachmentDescription;
}
} // namespace

namespace RHI::vulkan
{
GenericAttachment::GenericAttachment(Context & ctx, const TextureDescription & args,
                                     RHI::RenderBuffering buffering, RHI::SamplesCount samplesCount)
  : OwnedBy<Context>(ctx)
  , m_description(args)
  , m_samplesCount(samplesCount)
  , m_instancesCount(static_cast<uint32_t>(buffering))
{
  m_images.reserve(m_instancesCount);
  m_views.reserve(m_instancesCount);
  m_synchronizers.reserve(m_instancesCount);
}

GenericAttachment::~GenericAttachment()
{
  for (auto && view : m_views)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(view), nullptr);
  for (auto && memBlock : m_images)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(memBlock), nullptr);
}


//--------------------- IAttachment interface ----------------


std::shared_ptr<IAwaitable> GenericAttachment::DownloadImage(const DownloadImageArgs & args)
{
  return GetContext().GetTransferer(QueueType::Graphics).DownloadImage(*this, args);
}

size_t GenericAttachment::Size() const
{
  return std::accumulate(m_images.begin(), m_images.end(), static_cast<size_t>(0),
                         [](size_t acc, auto && img) { return acc + img.Size(); });
}

void GenericAttachment::BlitTo(ITexture * texture)
{
  if (auto * ptr = dynamic_cast<IInternalTexture *>(texture))
    GetContext()
      .GetTransferer(QueueType::Graphics)
      .BlitImageToImage(*ptr, *this, RHI::TextureRegion{});
}

void GenericAttachment::SetClearValue(float r, float g, float b, float a)
{
  m_clearValue.color = VkClearColorValue{r, g, b, a};
}

void GenericAttachment::SetClearValue(float depth, uint32_t stencil)
{
  m_clearValue.depthStencil = VkClearDepthStencilValue{depth, stencil};
}

TextureDescription GenericAttachment::GetDescription() const noexcept
{
  return m_description;
}

// -------------------- ITexture interface ------------------

VkImageView GenericAttachment::GetImageView() const noexcept
{
  return m_views[m_activeImage];
}

VkImageLayout GenericAttachment::GetLayout() const noexcept
{
  return m_synchronizers[m_activeImage].GetLayout();
}

VkImage GenericAttachment::GetHandle() const noexcept
{
  return m_images[m_activeImage].GetImage();
}

VkFormat GenericAttachment::GetInternalFormat() const noexcept
{
  return utils::CastInterfaceEnum2Vulkan<VkFormat>(m_description.format);
}

VkExtent3D GenericAttachment::GetInternalExtent() const noexcept
{
  return {m_description.extent[0], m_description.extent[1], m_description.extent[2]};
}

uint32_t GenericAttachment::GetMipLevelsCount() const noexcept
{
  return 1;
}

uint32_t GenericAttachment::GetLayersCount() const noexcept
{
  return 1;
}

VkImageType GenericAttachment::GetImageType() const noexcept
{
  assert(m_description.type == RHI::ImageType::Image2D);
  return VK_IMAGE_TYPE_2D;
}

VkImageViewType GenericAttachment::GetImageViewType() const noexcept
{
  return VK_IMAGE_VIEW_TYPE_2D;
}

details::Synchronizer & GenericAttachment::GetSynchronizer() & noexcept
{
  return m_synchronizers[m_activeImage];
}

//-------------------- IAttachment interface --------------------

void GenericAttachment::Invalidate(VkImageUsageFlags usage)
{
  if (m_changedSize || m_changedMSAA)
  {
    for (auto && view : m_views)
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(view), nullptr);
    for (auto && memBlock : m_images)
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(std::move(memBlock), nullptr);
    m_images.clear();
    m_views.clear();

    m_changedSize = false;
    m_changedMSAA = false;
    m_changedImagesCount = true;
  }

  if (m_changedImagesCount)
  {
    while (m_images.size() > m_instancesCount)
    {
      m_images.pop_back();
      m_synchronizers.pop_back();
      m_views.pop_back();
    }

    auto desiredMSAA = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(m_samplesCount);
    while (m_images.size() < m_instancesCount)
    {
      auto memoryBlock =
        GetContext().GetBuffersAllocator().AllocImage(m_description, usage, desiredMSAA);
      m_synchronizers.emplace_back(GetContext(), memoryBlock.GetImage());
      m_views.emplace_back(utils::CreateImageView(GetContext().GetGpuConnection().GetDevice(),
                                                  memoryBlock.GetImage(), GetInternalFormat(),
                                                  VK_IMAGE_VIEW_TYPE_2D,
                                                  CalcImageAspectByFormat(m_description.format)));
      m_images.push_back(std::move(memoryBlock));
    }
    m_changedImagesCount = false;
  }
}

std::pair<VkImageView, VkSemaphore> GenericAttachment::AcquireForRendering()
{
  m_renderingMutex.lock();
  m_activeImage = (m_activeImage + 1) % m_images.size();

  // TODO: Set layout for image
  return {GetImageView(), VK_NULL_HANDLE};
}

bool GenericAttachment::FinalRendering(VkSemaphore waitSemaphore)
{
  // TODO: set layout for image
  m_renderingMutex.unlock();
  return true;
}

uint32_t GenericAttachment::GetBuffering() const noexcept
{
  return m_instancesCount;
}

RHI::SamplesCount GenericAttachment::GetSamplesCount() const noexcept
{
  return m_samplesCount;
}

VkAttachmentDescription GenericAttachment::BuildDescription() const noexcept
{
  assert(!m_changedMSAA && !m_changedSize && !m_changedImagesCount);
  return BuildAttachmentDescription(m_description, m_samplesCount);
}

void GenericAttachment::Resize(const VkExtent2D & new_extent) noexcept
{
  m_description.extent = {static_cast<texel_t>(new_extent.width),
                          static_cast<texel_t>(new_extent.height), 1};
  m_changedSize = true;
}

} // namespace RHI::vulkan
