#pragma once
#include <list>
#include <vector>

#include "VulkanContext.hpp"

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{
struct FrameInFlight;
struct DefaultFramebuffer;

/// @brief vulkan implementation for renderer
struct Swapchain final : public ISwapchain
{
  static constexpr uint32_t InvalidImageIndex = -1;

  explicit Swapchain(const Context & ctx, const vk::SurfaceKHR surface);
  virtual ~Swapchain() override;

  /// @brief destroys old surface data like framebuffers, images, images_views, ets and creates new
  virtual void Invalidate() override;
  virtual ICommandBuffer * BeginFrame(std::array<float, 4> clearColorValue = {0.0f, 0.0f, 0.0f,
                                                                              0.0f}) override;
  virtual void EndFrame() override;
  virtual const IFramebuffer & GetDefaultFramebuffer() const & noexcept override;
  virtual std::pair<uint32_t, uint32_t> GetExtent() const override;

  virtual std::unique_ptr<ICommandBuffer> CreateCommandBuffer() const override;

  VkFormat GetImageFormat() const noexcept;
  vk::ImageView GetImageView(size_t idx) const noexcept;
  size_t GetBuffersCount() const noexcept;
  vk::SwapchainKHR GetHandle() const noexcept;

private:
  static constexpr uint32_t MaxFramesInFlight = 2;
  const Context & m_owner;
  vk::Queue m_presentQueue = VK_NULL_HANDLE;
  uint32_t m_presentQueueIndex;
  vk::Queue m_graphicsQueue = VK_NULL_HANDLE; // graphics queue
  uint32_t m_graphicsQueueIndex;

  /// presentaqtion data
  vk::SurfaceKHR m_surface;       ///< surface
  bool m_usePresentation = false; ///< flag of presentation usage, true if surface is valid
  std::unique_ptr<vkb::Swapchain> m_swapchain; ///< swapchain
  vk::CommandPool m_pool;

  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  std::array<std::unique_ptr<FrameInFlight>, MaxFramesInFlight> m_framesInFlight{nullptr};
  uint32_t m_activeFrame = 0;
  uint32_t m_activeImage = 0;

  std::unique_ptr<DefaultFramebuffer> m_defaultFramebuffer;

private:
  /// destroy and create new swapchain
  void InvalidateSwapchain();
  void InvalidateFramebuffer();
};

} // namespace RHI::vulkan
