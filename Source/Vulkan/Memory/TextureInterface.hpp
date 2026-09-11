#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::details
{
struct CommandBuffer;
struct Synchronizer;
} // namespace RHI::vulkan::details

namespace RHI::vulkan
{

struct IInternalTexture
{
  virtual ~IInternalTexture() = default;
  virtual VkImageView GetImageView() const noexcept = 0;
  virtual VkImageLayout GetLayout() const noexcept = 0;
  virtual VkImage GetHandle() const noexcept = 0;
  virtual VkFormat GetInternalFormat() const noexcept = 0;
  virtual VkExtent3D GetInternalExtent() const noexcept = 0;
  virtual uint32_t GetMipLevelsCount() const noexcept = 0;
  virtual uint32_t GetLayersCount() const noexcept = 0;
  virtual VkImageType GetImageType() const noexcept = 0;
  virtual VkImageViewType GetImageViewType() const noexcept = 0;
  virtual details::Synchronizer & GetSynchronizer() & noexcept = 0;
};

} // namespace RHI::vulkan
