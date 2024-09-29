#pragma once
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else //TODO UNIX
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#define ENABLE_VALIDATION_LAYERS

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
constexpr const char * apiFolder = ".vulkan";
constexpr const char * shaderExtension = ".spv";

namespace details
{
using InternalHandle = void *;
}

enum class QueueType
{
  Present,
  Graphics,
  Compute,
  Transfer
};

struct BuffersAllocator;

/// @brief context is object contains vulkan logical device. Also it provides access to vulkan functions
///			If rendering system uses several GPUs, you should create one context for each physical device
struct Context final : public IContext
{
  /// @brief constructor
  explicit Context(const SurfaceConfig & config, LoggingFunc log);
  /// @brief destructor
  ~Context() override;

  //-------------- IContext Inteface --------------
  virtual ISwapchain & GetSwapchain() & noexcept override { return *m_swapchain; };
  virtual const ISwapchain & GetSwapchain() const & noexcept override { return *m_swapchain; };
  virtual std::unique_ptr<IFramebuffer> CreateFramebuffer() const override;
  virtual std::unique_ptr<IPipeline> CreatePipeline(const IFramebuffer & framebuffer,
                                                    uint32_t subpassIndex) const override;

  virtual std::unique_ptr<IBufferGPU> AllocBuffer(size_t size, BufferGPUUsage usage,
                                                  bool mapped = false) const override;
  virtual void WaitForIdle() const override;

  const vk::Instance GetInstance() const;
  const vk::Device GetDevice() const;
  const vk::PhysicalDevice GetGPU() const;
  std::pair<uint32_t, VkQueue> GetQueue(QueueType type) const;
  uint32_t GetVulkanVersion() const;

  void Log(LogMessageStatus status, const std::string & message) const noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
  std::unique_ptr<ISwapchain> m_swapchain;
  std::unique_ptr<BuffersAllocator> m_allocator;
  LoggingFunc m_logFunc;

private:
  Context(const Context &) = delete;
  Context & operator=(const Context &) = delete;
};

} // namespace RHI::vulkan

namespace RHI::vulkan::utils
{

/// @brief creates semaphore, doesn't own it
vk::Semaphore CreateVkSemaphore(vk::Device device);
/// @brief creates fence, doesn't own it
vk::Fence CreateFence(vk::Device device, bool locked = false);
/// @brief creates command pool for thread, doesn't own it
vk::CommandPool CreateCommandPool(vk::Device device, uint32_t queue_family_index);

} // namespace RHI::vulkan::utils
