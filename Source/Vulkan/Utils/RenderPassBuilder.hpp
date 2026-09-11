#pragma once
#include <vector>

#include <Memory/Synchronizer.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct RenderPassBuilder final
{
  /// @brief caches attachments to state
  /// @return true if attachments really changed and state has changed too
  bool SetAttachments(std::span<const VkAttachmentDescription> attachments);

  /// @brief set subpasses info to state
  /// @param subpasses - array of built subpass description
  /// @param selfDependencies - indices in subpasses array. Declares subpasses which should have self-dependency
  ///                           note: if your subpass uses vkCmdPipelineBarrier, you must add at least one self-dependency
  void SetSubpasses(std::vector<VkSubpassDescription> && subpasses,
                    std::vector<uint32_t> && selfDependencies);

  VkRenderPass Make(const VkDevice & device) const;
  void Reset();

  BarrierInfo GetFinalLayoutForAttachment(uint32_t subpassIndex, uint32_t attachmentIndex) const;
  std::span<const VkAttachmentDescription> GetCachedAttachments() const noexcept;

private:
  std::vector<VkSubpassDescription> m_subpassDescriptions;
  std::vector<VkAttachmentDescription> m_attachments;
  std::vector<VkSubpassDependency> m_dependenciesGraph;
  std::vector<BarrierInfo> m_attachmentsUsageTable; // table subpassIdx*AttachmentIdx
};
} // namespace RHI::vulkan::utils
