#include "PipelineAttachmentsUsage.hpp"

static bool operator==(const VkAttachmentReference & ref1, const VkAttachmentReference & ref2) noexcept
{
  return std::memcmp(&ref1, &ref2, sizeof(VkAttachmentReference)) == 0;
}

namespace RHI::vulkan
{
bool PipelineAttachmentsUsage::UseDepthStencil() const noexcept
{
  return m_depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED;
}

void PipelineAttachmentsUsage::BindAttachment(ShaderAttachmentSlot slot, uint32_t idx)
{
  auto assignOrInsert =
    [](std::vector<VkAttachmentReference> & atts, const VkAttachmentReference & ref)
  {
    auto it = std::find(atts.begin(), atts.end(), ref);
    if (it == atts.end())
      atts.push_back(ref);
    else
      *it = ref;
  };

  if (slot & ShaderAttachmentSlot::Preserved)
  {
    m_preservedAttachments.push_back(idx);
    return;
  }
  if (slot & ShaderAttachmentSlot::Color)
  {
    assignOrInsert(m_colorAttachments,
                   VkAttachmentReference{idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    assignOrInsert(m_resolveAttachments,
                   VkAttachmentReference{VK_ATTACHMENT_UNUSED,
                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  }
  if (slot & ShaderAttachmentSlot::DepthStencil)
  {
    m_depthStencilAttachment =
      VkAttachmentReference{idx, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  }
  if (slot & ShaderAttachmentSlot::Input)
  {
    assignOrInsert(m_inputAttachments,
                   VkAttachmentReference{idx, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL});
  }
}

void PipelineAttachmentsUsage::BindResolver(uint32_t idx, uint32_t resolve_idx)
{
  m_resolveAttachments[resolve_idx].attachment = idx;
}

VkSubpassDescription PipelineAttachmentsUsage::BuildDescription(
  VkPipelineBindPoint bindPoint) const noexcept
{
  VkSubpassDescription subpassDescription{};
  subpassDescription.pipelineBindPoint = bindPoint;
  subpassDescription.colorAttachmentCount = static_cast<uint32_t>(m_colorAttachments.size());
  subpassDescription.pColorAttachments = m_colorAttachments.data();
  subpassDescription.inputAttachmentCount = static_cast<uint32_t>(m_inputAttachments.size());
  subpassDescription.pInputAttachments = m_inputAttachments.data();
  subpassDescription.pDepthStencilAttachment = UseDepthStencil() ? &m_depthStencilAttachment
                                                                 : nullptr;
  // here must be attachments that should be MSAA resolved
  subpassDescription.pResolveAttachments = m_resolveAttachments.data();
  subpassDescription.pPreserveAttachments = m_preservedAttachments.data();
  subpassDescription.preserveAttachmentCount = static_cast<uint32_t>(m_preservedAttachments.size());
  return subpassDescription;
}

void PipelineAttachmentsUsage::CollectAttachmentsUsageInfo(
  std::span<VkImageUsageFlags> result) const
{
  for (auto && ref : m_colorAttachments)
  {
    result[ref.attachment] |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }
  for (auto && ref : m_inputAttachments)
  {
    result[ref.attachment] |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  }
  if (UseDepthStencil())
    result[m_depthStencilAttachment.attachment] |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
}

} // namespace RHI::vulkan
