#include <vulkan/vulkan.hpp>

#include "Builders.hpp"

namespace RHI::vulkan::details //-------- RenderPass Builder -------------
{
struct SubpassLayout
{
  const VkAttachmentReference * GetData(ShaderImageSlot slot) const noexcept
  {
    switch (slot)
    {
      case ShaderImageSlot::Color:
        return m_colorAttachments.data();
      case ShaderImageSlot::Input:
        return m_inputAttachments.data();
      case ShaderImageSlot::DepthStencil:
        return UseDepthStencil() ? &m_depthStencilAttachment : nullptr;
      default:
        assert(false);
        return nullptr;
    }
  }

  uint32_t GetAttachmentsCount(ShaderImageSlot slot) const noexcept
  {
    switch (slot)
    {
      case ShaderImageSlot::Color:
        return static_cast<uint32_t>(m_colorAttachments.size());
      case ShaderImageSlot::Input:
        return static_cast<uint32_t>(m_inputAttachments.size());
      case ShaderImageSlot::DepthStencil:
        return UseDepthStencil() ? 1 : 0;
      default:
        assert(false);
        return 0;
    }
  }

  bool UseDepthStencil() const noexcept
  {
    return m_depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED;
  }

  void AddAttachment(ShaderImageSlot slot, uint32_t idx)
  {
    VkAttachmentReference ref{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED};
    if (slot == ShaderImageSlot::DepthStencil && UseDepthStencil())
      throw std::runtime_error("DepthStencil attachment is set");

    switch (slot)
    {
      case ShaderImageSlot::Color:
        ref = VkAttachmentReference{idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        m_colorAttachments.push_back(ref);
        break;
      case ShaderImageSlot::Input:
        ref =
          VkAttachmentReference{idx, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL};
        m_inputAttachments.push_back(ref);
        break;
      case ShaderImageSlot::DepthStencil:
        ref = VkAttachmentReference{idx, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
        m_depthStencilAttachment = ref;
        break;
      default:
        assert(false);
    }
  }

private:
  std::vector<VkAttachmentReference> m_colorAttachments;
  std::vector<VkAttachmentReference> m_inputAttachments;
  VkAttachmentReference m_depthStencilAttachment{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED};
};

void RenderPassBuilder::AddAttachment(const VkAttachmentDescription & description)
{
  m_attachments.push_back(description);
}

void RenderPassBuilder::AddSubpass(SubpassSlots slotsLayout)
{
  m_slots.push_back(slotsLayout);
}


vk::RenderPass RenderPassBuilder::Make(const vk::Device & device) const
{
  if (m_slots.empty() || m_attachments.empty())
    return VK_NULL_HANDLE;

  VkRenderPass renderPass = VK_NULL_HANDLE;

  std::vector<SubpassLayout> attachmentsForSubpasses;
  std::vector<VkSubpassDescription> subpasses;
  std::vector<VkSubpassDependency> dependencies;

  for (auto && subpassSlots : m_slots)
  {
    const uint32_t subpassIndex = static_cast<uint32_t>(subpasses.size());
    auto && attachments = attachmentsForSubpasses.emplace_back();
    for (auto && [slot, idx] : subpassSlots)
      attachments.AddAttachment(slot, idx);

    VkSubpassDependency dependencyInfo{};
    dependencyInfo.srcSubpass = subpassIndex == 0 ? VK_SUBPASS_EXTERNAL : subpassIndex - 1;
    dependencyInfo.dstSubpass = subpassIndex;
    dependencyInfo.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencyInfo.srcAccessMask = 0;
    dependencyInfo.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencyInfo.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.push_back(dependencyInfo);

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount =
      attachments.GetAttachmentsCount(ShaderImageSlot::Color);
    subpassDescription.pColorAttachments = attachments.GetData(ShaderImageSlot::Color);
    subpassDescription.inputAttachmentCount =
      attachments.GetAttachmentsCount(ShaderImageSlot::Input);
    subpassDescription.pInputAttachments = attachments.GetData(ShaderImageSlot::Input);
    subpassDescription.pDepthStencilAttachment = attachments.GetData(ShaderImageSlot::DepthStencil);
    subpasses.push_back(subpassDescription);
  }

  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
  renderPassCreateInfo.pAttachments = m_attachments.data();
  renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
  renderPassCreateInfo.pSubpasses = subpasses.data();
  renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassCreateInfo.pDependencies = dependencies.data();


  if (auto res = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create render pass");

  return vk::RenderPass(renderPass);
}

void RenderPassBuilder::Reset()
{
  m_attachments.clear();
  m_slots.clear();
  //m_usageFlags.clear();
}

} // namespace RHI::vulkan::details


namespace RHI::vulkan::details //--------------- Framebuffer builder ------------
{
void FramebufferBuilder::AddAttachment(const vk::ImageView & image)
{
  m_images.push_back(image);
}

vk::ImageView & FramebufferBuilder::SetAttachment(size_t idx) & noexcept
{
  return m_images[idx];
}

vk::Framebuffer FramebufferBuilder::Make(const vk::Device & device,
                                         const vk::RenderPass & renderPass,
                                         const VkExtent2D & extent) const
{
  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = static_cast<uint32_t>(m_images.size());
  framebufferInfo.pAttachments = reinterpret_cast<const VkImageView *>(m_images.data());
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
  framebufferInfo.layers = 1; // кол-во изображений
  VkFramebuffer framebuffer;
  if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
    throw std::runtime_error("failed to create framebuffer!");
  return vk::Framebuffer(framebuffer);
}

void FramebufferBuilder::Reset()
{
  m_images.clear();
}

} // namespace RHI::vulkan::details
