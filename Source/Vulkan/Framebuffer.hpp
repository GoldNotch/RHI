#pragma once
#include <list>

#include "VulkanContext.hpp"

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{
struct CommandBuffer;
struct Framebuffer;
struct Swapchain;

namespace details
{
struct RenderPassBuilder;
struct FramebufferBuilder;
} // namespace details


struct DefaultFramebuffer : public IFramebuffer
{
  explicit DefaultFramebuffer(const Context & ctx, Swapchain & swapchain,
                              VkSampleCountFlagBits samplesCount);
  virtual ~DefaultFramebuffer() override;

  virtual void Invalidate() override;
  virtual void SetExtent(uint32_t width, uint32_t height) override;
  //virtual void AddColorAttachment(/*ImageFormat*/) override;

  void BeginRenderPass(uint32_t activeFrame, const CommandBuffer & buffer,
                       const std::array<float, 4> & clearColorValue);
  void EndRenderPass(const CommandBuffer & buffer);

  virtual InternalObjectHandle GetRenderPass() const noexcept override;
  virtual InternalObjectHandle GetHandle() const noexcept override;

protected:
  const Context & m_owner;
  Swapchain & m_swapchain;

  constexpr static uint32_t InvalidIndex = static_cast<uint32_t>(-1);
  std::vector<std::unique_ptr<Framebuffer>> m_frames;
  uint32_t m_frameIndex = InvalidIndex;

  vk::RenderPass m_renderPass = VK_NULL_HANDLE;
  vk::Extent2D m_extent;
  std::unique_ptr<details::RenderPassBuilder> m_renderPassBuilder;
  bool m_invalidRenderPass : 1 = false;
  bool m_invalidFramebuffer : 1 = false;


private:
  void InvalidateFramebuffers();
  Framebuffer & CurFrame() & noexcept { return *m_frames[m_frameIndex]; }
  const Framebuffer & CurFrame() const & noexcept { return *m_frames[m_frameIndex]; }
};

} // namespace RHI::vulkan
