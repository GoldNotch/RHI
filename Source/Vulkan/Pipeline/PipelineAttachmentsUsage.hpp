#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
/// @brief Collects statistics about attachments used in VkSubpass and how they are used
struct PipelineAttachmentsUsage final
{
  PipelineAttachmentsUsage() = default;

  bool UseDepthStencil() const noexcept;
  void BindAttachment(ShaderAttachmentSlot slot, uint32_t idx);
  void BindResolver(uint32_t idx, uint32_t resolve_idx);
  VkSubpassDescription BuildDescription(VkPipelineBindPoint bindPoint) const noexcept;
  void CollectAttachmentsUsageInfo(std::span<VkImageUsageFlags> result) const;

private:
  std::vector<VkAttachmentReference> m_colorAttachments;
  std::vector<VkAttachmentReference> m_inputAttachments;
  std::vector<uint32_t> m_preservedAttachments;
  /// for MSAA. The same size as m_colorAttachments
  std::vector<VkAttachmentReference> m_resolveAttachments;
  VkAttachmentReference m_depthStencilAttachment{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED};
};
} // namespace RHI::vulkan
