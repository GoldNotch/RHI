#pragma once

#include <Pipeline/BaseDescriptor.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>


namespace RHI::vulkan
{

struct InputAttachmentUniform final : public details::BaseDescriptor
{
  explicit InputAttachmentUniform(Context & ctx, Pipeline & pipeline, LayoutIndex index,
                                  uint32_t attachmentIdx);
  virtual ~InputAttachmentUniform() override = default;


  virtual void UpdateDescriptorSet(std::span<const VkDescriptorSet> sets) const override;

public: // IResourceUser
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const override;
  virtual void SynchroniseResources(details::CommandBuffer & commands) const override;

public: // IInvalidable interface
  virtual void Invalidate() override;
  void SetInvalid();

private:
    uint32_t m_attachmentIndex = -1;
};

} // namespace RHI::vulkan
