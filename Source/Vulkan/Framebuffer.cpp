#include "Framebuffer.hpp"

#include <VkBootstrap.h>

#include "CommandBuffer.hpp"
#include "Swapchain.hpp"
#include "Utils/Builders.hpp"

namespace RHI::vulkan
{
struct Framebuffer final
{
  Framebuffer(const Context & ctx);
  ~Framebuffer();
  void Invalidate(const vk::RenderPass & renderPass, const VkExtent2D & extent);

  vk::Framebuffer GetFramebuffer() const noexcept { return m_framebuffer; }

private:
  friend struct DefaultFramebuffer;
  const Context & m_owner;

  vk::Framebuffer m_framebuffer = VK_NULL_HANDLE;
  std::unique_ptr<details::FramebufferBuilder> m_builder;
  bool m_invalidFramebuffer : 1 = false;

private:
  details::FramebufferBuilder & GetBuilder() & noexcept { return *m_builder; }
  void SetFramebufferInvalid() noexcept { m_invalidFramebuffer = true; }

private:
  Framebuffer(const Framebuffer &) = delete;
  Framebuffer & operator=(const Framebuffer &) = delete;
};

Framebuffer::Framebuffer(const Context & ctx)
  : m_owner(ctx)
  , m_builder(new details::FramebufferBuilder())
{
}

Framebuffer::~Framebuffer()
{
  if (!!m_framebuffer)
    vkDestroyFramebuffer(m_owner.GetDevice(), m_framebuffer, nullptr);
}

void Framebuffer::Invalidate(const vk::RenderPass & renderPass, const VkExtent2D & extent)
{
  if (m_invalidFramebuffer || !m_framebuffer)
  {
    auto new_framebuffer = m_builder->Make(m_owner.GetDevice(), renderPass, extent);
    if (!!m_framebuffer)
      vkDestroyFramebuffer(m_owner.GetDevice(), m_framebuffer, nullptr);
    m_framebuffer = new_framebuffer;
    m_invalidFramebuffer = false;
  }
}


DefaultFramebuffer::DefaultFramebuffer(const Context & ctx, Swapchain & swapchain,
                                       VkSampleCountFlagBits samplesCount)
  : m_owner(ctx)
  , m_swapchain(swapchain)
  , m_renderPassBuilder(new details::RenderPassBuilder())
{
  m_frames.reserve(m_swapchain.GetBuffersCount());
  for (size_t i = 0; i < m_swapchain.GetBuffersCount(); ++i)
    m_frames.emplace_back(std::make_unique<Framebuffer>(ctx));

  VkAttachmentDescription swapchainAttachment{};
  swapchainAttachment.format = m_swapchain.GetImageFormat();
  swapchainAttachment.samples = samplesCount; // MSAA
  swapchainAttachment.loadOp =
    VK_ATTACHMENT_LOAD_OP_CLEAR; // action for color/depth buffers in pass begins
  swapchainAttachment.storeOp =
    VK_ATTACHMENT_STORE_OP_STORE; // action for color/depth buffers in pass ends
  swapchainAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // action
  swapchainAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  swapchainAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  swapchainAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  m_renderPassBuilder->AddAttachment(swapchainAttachment);
  m_renderPassBuilder->AddSubpass({{ShaderImageSlot::Color, 0}});

  for (auto && frame : m_frames)
  {
    frame->GetBuilder().AddAttachment(VK_NULL_HANDLE);
    frame->SetFramebufferInvalid();
  }
  m_invalidFramebuffer = true;
}

DefaultFramebuffer::~DefaultFramebuffer()
{
  if (!!m_renderPass)
    vkDestroyRenderPass(m_owner.GetDevice(), m_renderPass, nullptr);
}

void DefaultFramebuffer::SetExtent(uint32_t width, uint32_t height)
{
  m_extent = VkExtent2D{width, height};
  m_invalidFramebuffer = true;
}

void DefaultFramebuffer::Invalidate()
{
  if (m_invalidRenderPass || !m_renderPass)
  {
    auto new_renderpass = m_renderPassBuilder->Make(m_owner.GetDevice());
    if (!!m_renderPass)
      vkDestroyRenderPass(m_owner.GetDevice(), m_renderPass, nullptr);
    m_renderPass = new_renderpass;
    m_invalidRenderPass = false;
    m_invalidFramebuffer = true;
  }

  InvalidateFramebuffers();
}

void DefaultFramebuffer::InvalidateFramebuffers()
{
  if (m_invalidFramebuffer)
  {
    size_t i = 0;
    for (auto && frame : m_frames)
    {
      frame->GetBuilder().SetAttachment(0) = m_swapchain.GetImageView(i++);
      frame->SetFramebufferInvalid();
      frame->Invalidate(m_renderPass, m_extent);
    }
    m_invalidFramebuffer = false;
  }
}

void DefaultFramebuffer::BeginRenderPass(uint32_t activeFrame, const CommandBuffer & buffer,
                                         const std::array<float, 4> & clearColorValue)
{
  m_frameIndex = activeFrame;
  VkClearValue clearValue{clearColorValue[0], clearColorValue[1], clearColorValue[2],
                          clearColorValue[3]};

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass;
  renderPassInfo.framebuffer = CurFrame().GetFramebuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = m_extent;
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = reinterpret_cast<const VkClearValue *>(&clearValue);

  vkCmdBeginRenderPass(buffer.GetHandle(), &renderPassInfo,
                       VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void DefaultFramebuffer::EndRenderPass(const CommandBuffer & buffer)
{
  vkCmdEndRenderPass(buffer.GetHandle());
}

InternalObjectHandle DefaultFramebuffer::GetRenderPass() const noexcept
{
  return static_cast<VkRenderPass>(m_renderPass);
}

InternalObjectHandle DefaultFramebuffer::GetHandle() const noexcept
{
  return static_cast<VkFramebuffer>(CurFrame().m_framebuffer);
}

} // namespace RHI::vulkan
