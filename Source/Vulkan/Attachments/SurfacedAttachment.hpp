#pragma once
#include <deque>
#include <vector>

#include <Attachments/GenericAttachment.hpp>
#include <Memory/Synchronizer.hpp>
#include <Surface.hpp>

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct SurfacedAttachment : public IAttachment,
                            public IInternalAttachment,
                            public OwnedBy<Context>
{
  explicit SurfacedAttachment(Context & ctx, Surface && surface, RHI::RenderBuffering buffering);
  virtual ~SurfacedAttachment() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // ITexture interface
  virtual std::shared_ptr<IAwaitable> DownloadImage(const DownloadImageArgs & args) override;
  virtual TextureDescription GetDescription() const noexcept override;
  virtual size_t Size() const override;
  virtual void SetClearValue(float r, float g, float b, float a) override;
  virtual void SetClearValue(float depth, uint32_t stencil) override;

public: //IInternalTexture interface
  virtual VkImageView GetImageView() const noexcept override;
  virtual VkImageLayout GetLayout() const noexcept override;
  virtual VkImage GetHandle() const noexcept override;
  virtual VkFormat GetInternalFormat() const noexcept override;
  virtual VkExtent3D GetInternalExtent() const noexcept override;
  virtual uint32_t GetMipLevelsCount() const noexcept override;
  virtual uint32_t GetLayersCount() const noexcept override;
  virtual VkImageType GetImageType() const noexcept override;
  virtual VkImageViewType GetImageViewType() const noexcept override;

public: // IAttachment interface
  virtual void BlitTo(ITexture * texture) override;

public: // IInternalAttachment interface
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() override;
  virtual void Invalidate(VkImageUsageFlags usage) override;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) override;
  virtual uint32_t GetBuffering() const noexcept override;
  virtual RHI::SamplesCount GetSamplesCount() const noexcept override;
  virtual VkAttachmentDescription BuildDescription() const noexcept override;
  virtual void Resize(const VkExtent2D & new_extent) noexcept;
  virtual details::Synchronizer & GetSynchronizer() & noexcept override;

protected:
  void DestroySwapchain() noexcept;

public:
  /// samples count for MSAA
  static constexpr RHI::SamplesCount g_samplesCount = RHI::SamplesCount::One;
  /// desired format for swapchain
  static constexpr RHI::ImageFormat g_imagesFormat = RHI::ImageFormat::RGBA8;

private:
  std::mutex m_renderingMutex;
  uint32_t m_desiredBuffering = 2; ///< desired instances of image

  std::vector<VkImage> m_images;                          ///< swapchain images
  std::vector<VkImageView> m_imageViews;                  /// attachment image view
  std::vector<VkSemaphore> m_imageAvailabilitySemaphores; /// owns
  std::vector<details::Synchronizer> m_synchronizers;
  uint32_t m_activeSemaphore = 0;               ///<
  uint32_t m_activeImage = g_InvalidImageIndex; ///< image index in rendering

  /// presentation data
  Surface m_surface;
  std::unique_ptr<vkb::Swapchain> m_swapchain; ///< swapchain
  bool m_invalidSwapchain = false;
};

} // namespace RHI::vulkan
