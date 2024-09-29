#include "Swapchain.hpp"

#include <format>

#include <VkBootstrap.h>

#include "CommandBuffer.hpp"
#include "Framebuffer.hpp"
#include "Utils/Builders.hpp"

namespace RHI::vulkan
{

struct FrameInFlight
{
  explicit FrameInFlight(const Context & ctx, vk::CommandPool pool);
  ~FrameInFlight();

  void WaitForRenderingComplete() const noexcept;
  VkSemaphore GetPresentSemaphore() const noexcept { return m_presentAvailableSemaphore; }
  VkSemaphore GetRenderSemaphore() const noexcept { return m_renderFinishedSemaphore; }

  CommandBuffer & BeginFrame() &;
  void EndFrame();

  CommandBuffer & GetCommandBuffer() & noexcept { return *m_buffer; }
  const CommandBuffer & GetCommandBuffer() const & noexcept { return *m_buffer; }

private:
  const Context & m_owner;

  vk::Queue m_graphicsQueue = VK_NULL_HANDLE; // graphics queue
  uint32_t m_graphicsQueueIndex;

  VkSemaphore m_presentAvailableSemaphore;
  VkSemaphore m_renderFinishedSemaphore;
  VkFence m_isRendering;

  std::unique_ptr<CommandBuffer> m_buffer;

private:
  void Submit();
};

FrameInFlight::FrameInFlight(const Context & ctx, vk::CommandPool pool)
  : m_owner(ctx)
{
  std::tie(m_graphicsQueueIndex, m_graphicsQueue) = ctx.GetQueue(QueueType::Graphics);
  m_presentAvailableSemaphore = utils::CreateVkSemaphore(ctx.GetDevice());
  m_renderFinishedSemaphore = utils::CreateVkSemaphore(ctx.GetDevice());
  m_isRendering = utils::CreateFence(ctx.GetDevice(), true /*locked*/);
  m_buffer =
    std::make_unique<CommandBuffer>(ctx.GetDevice(), pool, RHI::CommandBufferType::Executable);
}

FrameInFlight::~FrameInFlight()
{
  vkDestroySemaphore(m_owner.GetDevice(), m_presentAvailableSemaphore, nullptr);
  vkDestroySemaphore(m_owner.GetDevice(), m_renderFinishedSemaphore, nullptr);
  vkDestroyFence(m_owner.GetDevice(), m_isRendering, nullptr);
}

void FrameInFlight::WaitForRenderingComplete() const noexcept
{
  const VkFence fence = m_isRendering;
  vkWaitForFences(m_owner.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
}

CommandBuffer & FrameInFlight::BeginFrame() &
{
  vkResetFences(m_owner.GetDevice(), 1, &m_isRendering);
  m_buffer->Reset();
  m_buffer->BeginWritingInSwapchain();
  return *m_buffer;
}

void FrameInFlight::EndFrame()
{
  m_buffer->EndWriting();
  Submit();
}

void FrameInFlight::Submit()
{
  const VkSemaphore imgAvail = m_presentAvailableSemaphore;
  const VkSemaphore renderFinished = m_renderFinishedSemaphore;
  const VkCommandBuffer buffer = m_buffer->GetHandle();
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &imgAvail;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &buffer;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &renderFinished;

  if (auto res = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_isRendering); res != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");
}


Swapchain::Swapchain(const Context & ctx, const vk::SurfaceKHR surface)
  : m_owner(ctx)
  , m_surface(surface)
  , m_swapchain(std::make_unique<vkb::Swapchain>())
{
  std::tie(m_graphicsQueueIndex, m_graphicsQueue) = ctx.GetQueue(QueueType::Graphics);
  m_pool = utils::CreateCommandPool(ctx.GetDevice(), m_graphicsQueueIndex);

  std::tie(m_presentQueueIndex, m_presentQueue) = ctx.GetQueue(QueueType::Present);
  m_usePresentation = !!surface;

  InvalidateSwapchain();
  m_defaultFramebuffer = std::make_unique<DefaultFramebuffer>(ctx, *this, VK_SAMPLE_COUNT_1_BIT);
  InvalidateFramebuffer();

  for (auto && frame : m_framesInFlight)
    frame = std::make_unique<FrameInFlight>(ctx, m_pool);
}

Swapchain::~Swapchain()
{
  vkDestroyCommandPool(m_owner.GetDevice(), m_pool, nullptr);
  //vkDestroyRenderPass(context_owner.GetDevice(), render_pass, nullptr);
  m_swapchain->destroy_image_views(m_swapchainImageViews);
  vkb::destroy_swapchain(*m_swapchain);
  vkb::destroy_surface(m_owner.GetInstance(), m_surface);
}

void Swapchain::Invalidate()
{
  if (m_usePresentation)
  {
    m_owner.WaitForIdle();
    InvalidateSwapchain();
    InvalidateFramebuffer();
  }
}

void Swapchain::InvalidateSwapchain()
{
  if (m_usePresentation)
  {
    auto [presentIndex, presentQueue] = m_owner.GetQueue(QueueType::Present);
    auto [renderIndex, renderQueue] = m_owner.GetQueue(QueueType::Present);
    vkb::SwapchainBuilder swapchain_builder(m_owner.GetGPU(), m_owner.GetDevice(), m_surface,
                                            renderIndex, presentIndex);
    auto swap_ret = swapchain_builder.set_old_swapchain(*m_swapchain).build();
    if (!swap_ret)
    {
      throw std::runtime_error("Failed to create Vulkan swapchain - " + swap_ret.error().message());
    }

    if (!m_swapchainImageViews.empty())
      m_swapchain->destroy_image_views(m_swapchainImageViews);

    vkb::destroy_swapchain(*m_swapchain);

    *m_swapchain = swap_ret.value();
    m_swapchainImages = m_swapchain->get_images().value();
    m_swapchainImageViews = m_swapchain->get_image_views().value();
  }
  //else offscreen rendering
}

void Swapchain::InvalidateFramebuffer()
{
  auto && extent = m_swapchain->extent;
  m_defaultFramebuffer->SetExtent(extent.width, extent.height);
  m_defaultFramebuffer->Invalidate();
}

ICommandBuffer * Swapchain::BeginFrame(std::array<float, 4> clearColorValue)
{
  // select image view for drawing
  if (!m_usePresentation)
    return nullptr;

  auto && activeFrameInFLight = m_framesInFlight[m_activeFrame];
  activeFrameInFLight->WaitForRenderingComplete();

  uint32_t imageIndex = InvalidImageIndex;
  auto res = vkAcquireNextImageKHR(m_owner.GetDevice(), m_swapchain->swapchain, UINT64_MAX,
                                   activeFrameInFLight->GetPresentSemaphore(), VK_NULL_HANDLE,
                                   &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    Invalidate();
    return nullptr;
  }
  else if (res != VK_SUCCESS)
  {
    m_owner.Log(LogMessageStatus::LOG_ERROR,
                std::format("Failed to acquire swap chain image - {}", static_cast<uint32_t>(res)));
    return nullptr;
  }

  m_activeImage = imageIndex;

  auto && result = activeFrameInFLight->BeginFrame();
  m_defaultFramebuffer->BeginRenderPass(m_activeImage, activeFrameInFLight->GetCommandBuffer(),
                                        clearColorValue);
  return &result;
}

void Swapchain::EndFrame()
{
  auto && activeFrameInFLight = m_framesInFlight[m_activeFrame];
  m_defaultFramebuffer->EndRenderPass(activeFrameInFLight->GetCommandBuffer());
  activeFrameInFLight->EndFrame();

  const VkSwapchainKHR swapchains[] = {GetHandle()};
  const VkSemaphore renderFinishedSem = activeFrameInFLight->GetRenderSemaphore();
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderFinishedSem;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &m_activeImage;
  presentInfo.pResults = nullptr; // Optional
  auto res = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    Invalidate();
    return;
  }
  else if (res != VK_SUCCESS)
  {
    m_owner.Log(LogMessageStatus::LOG_ERROR,
                std::format("Failed to queue image in present - {}", static_cast<uint32_t>(res)));
  }

  m_activeFrame = (m_activeFrame + 1) % m_framesInFlight.size();
}

const IFramebuffer & Swapchain::GetDefaultFramebuffer() const & noexcept
{
  return *m_defaultFramebuffer;
}

std::pair<uint32_t, uint32_t> Swapchain::GetExtent() const
{
  return {m_swapchain->extent.width, m_swapchain->extent.height};
}

std::unique_ptr<ICommandBuffer> Swapchain::CreateCommandBuffer() const
{
  return std::make_unique<CommandBuffer>(m_owner.GetDevice(), m_pool,
                                         CommandBufferType::ThreadLocal);
}

VkFormat Swapchain::GetImageFormat() const noexcept
{
  return m_swapchain->image_format;
}

vk::SwapchainKHR Swapchain::GetHandle() const noexcept
{
  return m_swapchain->swapchain;
}

vk::ImageView Swapchain::GetImageView(size_t idx) const noexcept
{
  return m_swapchainImageViews[idx];
}

size_t Swapchain::GetBuffersCount() const noexcept
{
  return m_swapchain->image_count;
}


} // namespace RHI::vulkan
