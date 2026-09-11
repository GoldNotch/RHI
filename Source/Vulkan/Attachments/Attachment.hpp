#pragma once

#include <Memory/TextureInterface.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
static constexpr uint32_t g_InvalidImageIndex = -1;

struct IInternalAttachment : public IInternalTexture
{
  virtual ~IInternalAttachment() = default;
  virtual void Invalidate(VkImageUsageFlags usage) = 0;
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() = 0;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) = 0;
  virtual uint32_t GetBuffering() const noexcept = 0;
  virtual RHI::SamplesCount GetSamplesCount() const noexcept = 0;
  // Rename to AddAttachmentDescriptionTo
  virtual VkAttachmentDescription BuildDescription() const noexcept = 0;
  virtual void Resize(const VkExtent2D & new_extent) noexcept = 0;
  VkClearValue GetClearValue() const noexcept { return m_clearValue; }

protected:
  VkClearValue m_clearValue;
};

} // namespace RHI::vulkan
