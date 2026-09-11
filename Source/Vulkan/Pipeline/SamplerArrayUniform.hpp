#pragma once

#include <Memory/TextureInterface.hpp>
#include <Pipeline/BaseDescriptor.hpp>
#include <RHI.hpp>
#include <Utils/SamplerBuilder.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{

struct SamplerArrayUniform final : public ISamplerArrayUniformDescriptor,
                                   public details::BaseDescriptor
{
  explicit SamplerArrayUniform(Context & ctx, Pipeline & pipeline, size_t size,
                               VkDescriptorType type, LayoutIndex index, uint32_t arrayIndex = 0);
  virtual ~SamplerArrayUniform() override;
  SamplerArrayUniform(SamplerArrayUniform && rhs) noexcept;
  SamplerArrayUniform & operator=(SamplerArrayUniform && rhs) noexcept;

public: // ISamplerDescriptor interface
  virtual void SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                           RHI::TextureWrapping wWrap) noexcept override;
  virtual void SetFilter(RHI::TextureFilteration minFilter,
                         RHI::TextureFilteration magFilter) noexcept override;

public:
  virtual void UpdateDescriptorSet(std::span<const VkDescriptorSet> sets) const override;


public: // IResourceUser
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const override;
  virtual void SynchroniseResources(SynchronizationFilter filter, details::CommandBuffer & commands) const override;

public: // IUniformDescriptor interface
  virtual uint32_t GetSet() const noexcept override { return BaseDescriptor::GetSet(); }
  virtual uint32_t GetBinding() const noexcept override { return BaseDescriptor::GetBinding(); }
  virtual uint32_t GetArrayIndex() const noexcept override
  {
    return BaseDescriptor::GetArrayIndex();
  }

public: // ISamplerArrayUniformDescriptor interface
  virtual void AssignImage(uint32_t index, ITexture * image) override;

public: // IInvalidable interface
  virtual void Invalidate() override;
  void SetInvalid();

public: // public internal API
  VkSampler GetHandle() const noexcept;
  using BaseDescriptor::GetDescriptorType;

private:
  mutable std::atomic_bool m_shouldUpdate = false;
  std::vector<IInternalTexture *> m_boundTextures;
  VkSampler m_sampler = VK_NULL_HANDLE;
  utils::SamplerBuilder m_builder;
  bool m_invalidSampler = true;
};

} // namespace RHI::vulkan
