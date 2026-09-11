#include "SurfacedAttachment.hpp"

#include <format>

#include <ImageUtils/InternalImageTraits.hpp>
#include <Utils/CastHelper.hpp>
#include <VkBootstrap.h>
#include <VulkanContext.hpp>

#include "../RenderPass/RenderPass.hpp"
#include "../Utils/SemaphoreBuilder.hpp"

namespace RHI::vulkan
{

static constexpr VkSurfaceFormatKHR g_vkFormat{utils::CastInterfaceEnum2Vulkan<VkFormat>(
                                                 SurfacedAttachment::g_imagesFormat),
                                               VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

SurfacedAttachment::SurfacedAttachment(Context & ctx, Surface && surface,
                                       RHI::RenderBuffering buffering)
  : OwnedBy<Context>(ctx)
  , m_surface(std::move(surface))
  , m_swapchain(std::make_unique<vkb::Swapchain>())
  , m_desiredBuffering(static_cast<uint32_t>(buffering))
{
}

SurfacedAttachment::~SurfacedAttachment()
{
  DestroySwapchain();
}

std::shared_ptr<IAwaitable> SurfacedAttachment::DownloadImage(const DownloadImageArgs & args)
{
  return GetContext().GetTransferer(QueueType::Graphics).DownloadImage(*this, args);
}

TextureDescription SurfacedAttachment::GetDescription() const noexcept
{
  TextureDescription description{};
  {
    description.mipLevels = 1;
    description.type = RHI::ImageType::Image2D;
    auto extent = GetInternalExtent();
    description.extent = {static_cast<texel_t>(extent.width), static_cast<texel_t>(extent.height),
                          static_cast<texel_t>(extent.depth)};
    description.format = g_imagesFormat;
  }
  return description;
}

size_t SurfacedAttachment::Size() const
{
  return RHI::utils::GetSizeOfImage(GetInternalExtent(), GetInternalFormat());
}

void SurfacedAttachment::SetClearValue(float r, float g, float b, float a)
{
  m_clearValue.color = VkClearColorValue{r, g, b, a};
}

void SurfacedAttachment::SetClearValue(float depth, uint32_t stencil)
{
  m_clearValue.depthStencil = VkClearDepthStencilValue{depth, stencil};
}

// -------------------- ITexture interface ---------------------

VkImageView SurfacedAttachment::GetImageView() const noexcept
{
  return m_imageViews[m_activeImage];
}

VkImageLayout SurfacedAttachment::GetLayout() const noexcept
{
  return m_synchronizers[m_activeImage].GetLayout();
}

VkImage SurfacedAttachment::GetHandle() const noexcept
{
  return m_images[m_activeImage];
}

VkFormat SurfacedAttachment::GetInternalFormat() const noexcept
{
  return m_swapchain && m_swapchain->swapchain ? m_swapchain->image_format : g_vkFormat.format;
}

VkExtent3D SurfacedAttachment::GetInternalExtent() const noexcept
{
  VkExtent2D result{0, 0};
  if (m_swapchain && m_swapchain->swapchain)
  {
    result = m_swapchain->extent;
  }
  else
  {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GetContext().GetGpuConnection().GetGPU(), m_surface,
                                              &surfaceCapabilities);
    result = surfaceCapabilities.currentExtent;
  }
  return {result.width, result.height, 1};
}

uint32_t SurfacedAttachment::GetMipLevelsCount() const noexcept
{
  return 1;
}

uint32_t SurfacedAttachment::GetLayersCount() const noexcept
{
  return 1;
}

VkImageType SurfacedAttachment::GetImageType() const noexcept
{
  return VK_IMAGE_TYPE_2D;
}

VkImageViewType SurfacedAttachment::GetImageViewType() const noexcept
{
  return VK_IMAGE_VIEW_TYPE_2D;
}

void SurfacedAttachment::BlitTo(ITexture * texture)
{
  //if (auto * ptr = dynamic_cast<RHI::vulkan::IInternalTexture *>(texture))
  //  GetContext().GetTransferer().BlitImageToImage(*ptr, *this);
}

// --------------------- IAttachment interface ----------------------

void SurfacedAttachment::Invalidate(VkImageUsageFlags usage)
{
  if (m_invalidSwapchain || !m_swapchain->swapchain)
  {
    auto [renderIndex, renderQueue] = GetContext().GetGpuConnection().GetQueue(QueueType::Graphics);
    auto [presentIndex, _] = GetContext().GetGpuConnection().GetQueue(QueueType::Present);
    vkb::SwapchainBuilder swapchain_builder(GetContext().GetGpuConnection().GetGPU(),
                                            GetContext().GetGpuConnection().GetDevice(), m_surface,
                                            renderIndex, presentIndex);
    if (m_desiredBuffering != g_InvalidImageIndex)
      swapchain_builder.set_required_min_image_count(m_desiredBuffering);
    swapchain_builder.set_desired_format(g_vkFormat);
    swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
    auto swap_ret = swapchain_builder.set_old_swapchain(*m_swapchain).build();
    if (!swap_ret)
      throw std::runtime_error("Failed to create Vulkan swapchain - " + swap_ret.error().message());

    DestroySwapchain();

    *m_swapchain = swap_ret.value();
    m_images = m_swapchain->get_images().value();
    m_imageViews = m_swapchain->get_image_views().value();
    for (auto && view : m_imageViews)
      m_imageAvailabilitySemaphores.push_back(
        utils::SemaphoreBuilder().Make(GetContext().GetGpuConnection().GetDevice()));

    m_synchronizers.reserve(m_images.size());
    for (auto image : m_images)
      m_synchronizers.emplace_back(GetContext(), image);

    // reset invalid flags
    m_invalidSwapchain = false;
    m_desiredBuffering = g_InvalidImageIndex;
  }
}

std::pair<VkImageView, VkSemaphore> SurfacedAttachment::AcquireForRendering()
{
  m_renderingMutex.lock();
  VkSemaphore signalSemaphore = m_imageAvailabilitySemaphores[m_activeSemaphore];
  uint32_t imageIndex = g_InvalidImageIndex;
  auto res = vkAcquireNextImageKHR(GetContext().GetGpuConnection().GetDevice(),
                                   m_swapchain->swapchain, UINT64_MAX, signalSemaphore,
                                   VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    m_renderingMutex.unlock();
    m_invalidSwapchain = true;
    return {VK_NULL_HANDLE, VK_NULL_HANDLE};
  }
  else if (res != VK_SUCCESS)
  {
    m_renderingMutex.unlock();
    throw std::runtime_error(
      std::format("Failed to acquire swap chain image - {}", static_cast<uint32_t>(res)));
  }
  m_activeImage = imageIndex;
  return {GetImageView(), signalSemaphore};
}

bool SurfacedAttachment::FinalRendering(VkSemaphore waitSemaphore)
{
  auto [_, presentQueue] = GetContext().GetGpuConnection().GetQueue(QueueType::Present);
  const VkSwapchainKHR swapchains[] = {m_swapchain->swapchain};
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = !!waitSemaphore ? 1 : 0;
  presentInfo.pWaitSemaphores = &waitSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &m_activeImage;
  presentInfo.pResults = nullptr; // Optional
  auto res = vkQueuePresentKHR(presentQueue, &presentInfo);
  m_activeSemaphore = (m_activeSemaphore + 1u) % m_imageAvailabilitySemaphores.size();
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    m_renderingMutex.unlock();
    Invalidate(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    return false;
  }
  else if (res != VK_SUCCESS)
  {
    m_renderingMutex.unlock();
    throw std::runtime_error(
      std::format("Failed to queue image in present - {}", static_cast<uint32_t>(res)));
  }
  m_renderingMutex.unlock();
  return true;
}

uint32_t SurfacedAttachment::GetBuffering() const noexcept
{
  return m_swapchain ? m_swapchain->image_count : 0;
}

RHI::SamplesCount SurfacedAttachment::GetSamplesCount() const noexcept
{
  return g_samplesCount;
}

VkAttachmentDescription SurfacedAttachment::BuildDescription() const noexcept
{
  VkAttachmentDescription description{};
  {
    description.format = GetInternalFormat();
    description.samples = utils::CastInterfaceEnum2Vulkan<VkSampleCountFlagBits>(g_samplesCount);
    description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  }
  return description;
}

void SurfacedAttachment::Resize(const VkExtent2D & new_extent) noexcept
{
  // do nothing because resizing handled in AcquireForRend
  m_invalidSwapchain = true;
}

details::Synchronizer & SurfacedAttachment::GetSynchronizer() & noexcept
{
  return m_synchronizers[m_activeImage];
}

// ---------------------------- Private -----------------

void SurfacedAttachment::DestroySwapchain() noexcept
{
  GetContext().WaitForIdle();
  if (!m_imageViews.empty())
    m_swapchain->destroy_image_views(m_imageViews);
  vkb::destroy_swapchain(*m_swapchain);
  // we can delete semaphore directly, because we have waited for gpu's idle
  for (auto sem : m_imageAvailabilitySemaphores)
    vkDestroySemaphore(GetContext().GetGpuConnection().GetDevice(), sem, nullptr);
  m_images.clear();
  m_imageViews.clear();
  m_imageAvailabilitySemaphores.clear();
  m_synchronizers.clear();
}

} // namespace RHI::vulkan
