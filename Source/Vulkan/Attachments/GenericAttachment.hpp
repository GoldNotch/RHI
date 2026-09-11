#pragma once
#include <mutex>

#include <Attachments/Attachment.hpp>
#include <Memory/MemoryBlock.hpp>
#include <Memory/Synchronizer.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct GenericAttachment : public IAttachment,
                           public IInternalAttachment,
                           public OwnedBy<Context>
{
  explicit GenericAttachment(Context & ctx, const TextureDescription & m_description,
                             RHI::RenderBuffering buffering, RHI::SamplesCount samplesCount);
  virtual ~GenericAttachment() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IAttachment interface
  virtual std::shared_ptr<IAwaitable> DownloadImage(const DownloadImageArgs & args) override;
  virtual TextureDescription GetDescription() const noexcept override;
  /// @brief Get size of image in bytes
  virtual size_t Size() const override;
  virtual void BlitTo(ITexture * texture) override;
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
  virtual details::Synchronizer & GetSynchronizer() & noexcept override;

public: // IInternalAttachment interface
  virtual void Invalidate(VkImageUsageFlags usage) override;
  virtual std::pair<VkImageView, VkSemaphore> AcquireForRendering() override;
  virtual bool FinalRendering(VkSemaphore waitSemaphore) override;
  virtual uint32_t GetBuffering() const noexcept override;
  virtual RHI::SamplesCount GetSamplesCount() const noexcept override;
  virtual VkAttachmentDescription BuildDescription() const noexcept override;
  virtual void Resize(const VkExtent2D & new_extent) noexcept override;

protected:
  std::mutex m_renderingMutex;      ///< mutex, because you can't enter in rendering mode twice
  TextureDescription m_description; ///< description of image, all main params for image

  std::vector<memory::MemoryBlock> m_images; ///< memory for image instances
  std::vector<VkImageView> m_views;
  std::vector<details::Synchronizer> m_synchronizers;
  uint32_t m_activeImage = 0;

  uint32_t m_instancesCount = 0;
  const RHI::SamplesCount m_samplesCount = RHI::SamplesCount::One;
  bool m_changedImagesCount = true;
  bool m_changedSize = false;
  bool m_changedMSAA = false;
};

} // namespace RHI::vulkan
