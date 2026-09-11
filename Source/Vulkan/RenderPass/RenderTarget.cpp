#include "RenderTarget.hpp"

#include <algorithm>

#include <CommandsExecution/CommandBuffer.hpp>
#include <RenderPass/RenderPass.hpp>
#include <VkBootstrap.h>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

RenderTarget::RenderTarget(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

RenderTarget::~RenderTarget()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_framebuffer, nullptr);
}

RenderTarget::RenderTarget(RenderTarget && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::swap(m_boundRenderPass, rhs.m_boundRenderPass);
  std::swap(m_attachedImages, rhs.m_attachedImages);
  std::swap(m_extent, rhs.m_extent);
  std::swap(m_framebuffer, rhs.m_framebuffer);
  std::swap(m_builder, rhs.m_builder);
  std::swap(m_invalidFramebuffer, rhs.m_invalidFramebuffer);
}

RenderTarget & RenderTarget::operator=(RenderTarget && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_boundRenderPass, rhs.m_boundRenderPass);
    std::swap(m_attachedImages, rhs.m_attachedImages);
    std::swap(m_extent, rhs.m_extent);
    std::swap(m_framebuffer, rhs.m_framebuffer);
    std::swap(m_builder, rhs.m_builder);
    std::swap(m_invalidFramebuffer, rhs.m_invalidFramebuffer);
  }
  return *this;
}

void RenderTarget::Invalidate()
{
  assert(m_boundRenderPass);
  if (m_invalidFramebuffer || !m_framebuffer)
  {
    m_builder.Reset();
    for (uint32_t i = 0; i < m_attachedImages.size(); ++i)
      m_builder.BindAttachment(i, m_attachedImages[i]);

    auto new_framebuffer =
      m_builder.Make(GetContext().GetGpuConnection().GetDevice(), m_boundRenderPass, m_extent);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_framebuffer, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkFramebuffer({}) has been rebuilt - {}",
                     static_cast<void *>(m_framebuffer), static_cast<void *>(new_framebuffer));
    m_framebuffer = new_framebuffer;
    m_invalidFramebuffer = false;
  }
}

void RenderTarget::BindRenderPass(const VkRenderPass & renderPass) noexcept
{
  if (renderPass != m_boundRenderPass)
  {
    m_boundRenderPass = renderPass;
    m_invalidFramebuffer = true;
  }
}

void RenderTarget::SetExtent(const VkExtent3D & extent) noexcept
{
  m_extent = extent;
}

std::span<const VkClearValue> RenderTarget::GetClearValues() const noexcept
{
  return m_clearValues;
}

std::span<const VkImageView> RenderTarget::GetImageViews() const noexcept
{
  return m_attachedImages;
}

std::span<const VkSemaphore> RenderTarget::GetImageAvailableForRenderSemaphores() const noexcept
{
  return m_imageAvailabilitySemaphores;
}

void RenderTarget::SetAttachments(std::vector<VkImageView> && views,
                                  std::vector<VkClearValue> && clearValues,
                                  std::vector<VkSemaphore> && imageSemaphores) noexcept
{
  if (views != m_attachedImages)
    m_invalidFramebuffer = true;
  m_attachedImages = std::move(views);
  m_clearValues = std::move(clearValues);
  m_imageAvailabilitySemaphores = std::move(imageSemaphores);
}

size_t RenderTarget::GetAttachmentsCount() const noexcept
{
  return m_attachedImages.size();
}

void RHI::vulkan::RenderTarget::ClearAttachments() noexcept
{
  m_attachedImages.clear();
  m_clearValues.clear();
  m_invalidFramebuffer = true;
}


} // namespace RHI::vulkan
