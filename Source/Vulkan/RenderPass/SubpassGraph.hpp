#pragma once
#include <vector>

#include <Memory/Synchronizer.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct IInternalAttachment;
}

namespace RHI::vulkan
{
enum class SubpassIndex : int32_t
{
  initialRenderPass = -1,
  finalRenderPass = std::numeric_limits<int32_t>::max(),
  Subpass0 = 0,
  Subpass1 = 1,
  Subpass2,
  Subpass3,
  Subpass4,
  Subpass5,
  //...
};

struct SubpassGraph final
{
  /// @brief caches attachments to state
  /// @return true if attachments really changed and state has changed too
  bool SetAttachments(std::span<const VkAttachmentDescription> attachments);

  /// @brief set subpasses info to state
  /// @param subpasses - array of built subpass description
  /// @param selfDependencies - indices in subpasses array. Declares subpasses which should have self-dependency
  ///                           note: if your subpass uses vkCmdPipelineBarrier, you must add at least one self-dependency
  void BuildGraph(std::vector<VkSubpassDescription> && subpasses,
                  std::vector<SubpassIndex> && selfDependencies);
  void ResetGraph();

  VkRenderPass MakeRenderPass(const VkDevice & device) const;

  void SynchronizeAttachmentsDuringRenderPass(SubpassIndex subpassIndex,
                                              std::span<IInternalAttachment *> attachments);
  std::span<const VkAttachmentDescription> GetCachedAttachments() const noexcept;

private:
  std::vector<VkSubpassDescription> m_subpassDescriptions;
  std::vector<VkAttachmentDescription> m_attachments;
  std::vector<VkSubpassDependency> m_dependenciesGraph;
  std::vector<BarrierInfo> m_attachmentsUsageTable; // table subpassIdx*AttachmentIdx

private:
  size_t GetBarrierRowIndex(SubpassIndex idx) const noexcept;
  std::span<const BarrierInfo> GetBarriersRow(SubpassIndex idx) const noexcept;
  std::span<BarrierInfo> GetBarriersRow(SubpassIndex idx) noexcept;
  void BuildSubpassGraph();
  void BuildDependencyGraph(std::span<SubpassIndex> selfDependencies);
};
} // namespace RHI::vulkan
