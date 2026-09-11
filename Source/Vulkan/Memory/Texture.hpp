#pragma once
#include <Memory/MemoryBlock.hpp>
#include <Private/OwnedBy.hpp>
#include <Memory/Synchronizer.hpp>
#include <Memory/TextureInterface.hpp>
#include <RHI.hpp>

namespace RHI::vulkan
{
struct Context;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct Texture : public ITexture,
                 public IInternalTexture,
                 public OwnedBy<Context>
{
  Texture(Context & ctx, const TextureDescription & args);
  virtual ~Texture() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  RESTRICTED_COPY(Texture);

public: // ITexture interface
  virtual std::shared_ptr<IAwaitable> UploadImage(const UploadImageArgs & args) override;
  virtual std::shared_ptr<IAwaitable> DownloadImage(const DownloadImageArgs & args) override;
  virtual std::shared_ptr<IAwaitable> GenerateMipmaps() override;

  virtual TextureDescription GetDescription() const noexcept override;
  virtual size_t Size() const override;
  //virtual void SetSwizzle() = 0;
  virtual void BlitTo(ITexture * texture) override;

public: // IInternalTexture interface
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

private:
  TextureDescription m_description;
  memory::MemoryBlock m_memBlock;
  VkImageView m_view = VK_NULL_HANDLE;
  details::Synchronizer m_synchronizer;
};
} // namespace RHI::vulkan
