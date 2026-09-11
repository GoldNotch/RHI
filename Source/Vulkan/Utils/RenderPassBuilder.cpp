#include "RenderPassBuilder.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <span>

/// @brief Compare operator for VkAttachmentDescription
static inline bool operator==(const VkAttachmentDescription & lhs,
                              const VkAttachmentDescription & rhs) noexcept
{
  return std::memcmp(&lhs, &rhs, sizeof(VkAttachmentDescription)) == 0;
}

namespace
{
RHI::vulkan::BarrierInfo CalcAttachmentBarrier(const RHI::vulkan::BarrierInfo & prevBarrier,
                                               VkImageLayout newLayout) noexcept
{
  VkPipelineStageFlags stage = 0;
  VkAccessFlags access = 0;

  switch (newLayout)
  {
    case VK_IMAGE_LAYOUT_UNDEFINED:
      // Should not transition to undefined in a dependency
      stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      access = 0;
      break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
      stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
      stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      access = VK_ACCESS_SHADER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      access = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      access = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_GENERAL:
      // General layout could be used for many purposes, include common stages
      stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      access = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      access = 0; // Present doesn't write to the image
      break;

      //case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_READ_ONLY_OPTIMAL:
      //    stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      //    access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      //    break;

    default:
      // Conservative fallback for unknown layouts
      stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      access = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
      break;
  }

  return RHI::vulkan::BarrierInfo{stage, access, newLayout};
}

bool IsFramebufferSpaceStage(VkPipelineStageFlags stage) noexcept
{
  const VkPipelineStageFlags framebufferStages =
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  return static_cast<bool>(stage & framebufferStages);
}

std::vector<RHI::vulkan::BarrierInfo> BuildAttachmentUsageTable(
  std::span<const VkSubpassDescription> descriptions,
  std::span<const VkAttachmentDescription> attachments)
{
  std::vector<RHI::vulkan::BarrierInfo> layoutsTable;
  size_t attachmentsCount = attachments.size();
  size_t subpassesCount = descriptions.size();
  layoutsTable.resize((subpassesCount + 2) * attachmentsCount);

  for (size_t i = 0; i < attachmentsCount; ++i)
  {
    // initial barrier for attachment
    layoutsTable[i] = RHI::vulkan::BarrierInfo{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                                               VK_ACCESS_2_NONE, attachments[i].initialLayout};
    // final barrier for attachment
    layoutsTable[(subpassesCount + 1) + i] =
      RHI::vulkan::BarrierInfo{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                               attachments[i].finalLayout};
  }

  std::span<const RHI::vulkan::BarrierInfo> prevRow(layoutsTable.begin(), attachmentsCount);

  for (size_t i = 0; i < subpassesCount; ++i)
  {
    auto && description = descriptions[i];
    std::span<const VkAttachmentReference> colorAttachments(description.pColorAttachments,
                                                            description.colorAttachmentCount);
    std::span<const VkAttachmentReference>
      depthStencilAttachments(description.pDepthStencilAttachment,
                              description.pDepthStencilAttachment ? 1 : 0);
    std::span<const VkAttachmentReference> inputAttachments(description.pInputAttachments,
                                                            description.inputAttachmentCount);
    std::span<RHI::vulkan::BarrierInfo> layoutsRow(layoutsTable.begin() +
                                                     (i + 1) * attachmentsCount,
                                                   attachmentsCount);
    for (auto && ref : colorAttachments)
      layoutsRow[ref.attachment] = CalcAttachmentBarrier(prevRow[ref.attachment], ref.layout);
    for (auto && ref : depthStencilAttachments)
      layoutsRow[ref.attachment] = CalcAttachmentBarrier(prevRow[ref.attachment], ref.layout);
    for (auto && ref : inputAttachments)
      layoutsRow[ref.attachment] = CalcAttachmentBarrier(prevRow[ref.attachment], ref.layout);

    prevRow = layoutsRow;
  }
  return layoutsTable;
}

std::vector<VkSubpassDependency> BuildDependenciesGraph(
  std::span<const RHI::vulkan::BarrierInfo> attachmentsUsageInfo, size_t attachmentsCount,
  std::span<const VkSubpassDescription> descriptions, std::span<uint32_t> selfDependencies)
{
  size_t subpassesCount = descriptions.size();
  std::vector<VkSubpassDependency> dependencies;
  dependencies.reserve(subpassesCount + selfDependencies.size());

  std::span<const RHI::vulkan::BarrierInfo> prevRow(attachmentsUsageInfo.begin(), attachmentsCount);
  for (size_t i = 0; i < subpassesCount; ++i)
  {
    std::span<const RHI::vulkan::BarrierInfo> row(attachmentsUsageInfo.begin() +
                                                    (i + 1) * attachmentsCount,
                                                  attachmentsCount);
    auto depInfo = dependencies.emplace_back();
    depInfo.srcSubpass = i == 0 ? VK_SUBPASS_EXTERNAL : i - 1;
    depInfo.dstSubpass = i;
    if (i == 0)
    {
      for (auto && barrier : prevRow)
      {
        depInfo.srcStageMask |= barrier.currentStage;
        depInfo.srcAccessMask |= barrier.requiredAccess;
      }
    }
    else
    {
      depInfo.srcStageMask = dependencies[i - 1].dstStageMask;
      depInfo.srcAccessMask = dependencies[i - 1].dstAccessMask;
    }
    for (auto && barrier : row)
    {
      depInfo.dstStageMask |= barrier.currentStage;
      depInfo.dstAccessMask |= barrier.requiredAccess;
    }
    prevRow = row;
  }

  for (uint32_t i : selfDependencies)
  {
    auto && description = descriptions[i];
    std::span<const VkAttachmentReference> colorAttachments(description.pColorAttachments,
                                                            description.colorAttachmentCount);
    std::span<const VkAttachmentReference>
      depthStencilAttachments(description.pDepthStencilAttachment,
                              description.pDepthStencilAttachment ? 1 : 0);
    std::span<const VkAttachmentReference> inputAttachments(description.pInputAttachments,
                                                            description.inputAttachmentCount);
    VkSubpassDependency selfDependency{};
    selfDependency.srcSubpass = i;
    selfDependency.dstSubpass = i;
    selfDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    // Handle color attachments: writes happen at COLOR_ATTACHMENT_OUTPUT,
    // reads (if any) also happen at COLOR_ATTACHMENT_OUTPUT
    if (!colorAttachments.empty())
    {
      selfDependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      selfDependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      selfDependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      selfDependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    // Handle depth/stencil attachments: writes and reads happen at
    // EARLY_FRAGMENT_TESTS and LATE_FRAGMENT_TESTS
    if (!depthStencilAttachments.empty())
    {
      selfDependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                     VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      selfDependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      selfDependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                     VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      selfDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    // Handle input attachments: reads happen at FRAGMENT_SHADER stage
    if (!inputAttachments.empty())
    {
      // Input attachments are read in the fragment shader.
      // For a self-dependency, we need to synchronize the writes to those
      // attachments (which could be color/depth writes in the same subpass)
      // with the reads in the fragment shader.
      selfDependency.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      selfDependency.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

      // If there are no color or depth attachments, we still need a source
      // for the dependency. Use the same fragment shader stage as source
      // with no specific access (just execution dependency).
      if (colorAttachments.empty() && depthStencilAttachments.empty())
      {
        selfDependency.srcStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        // No srcAccessMask needed for pure execution dependency
      }
    }
    assert(!IsFramebufferSpaceStage(selfDependency.srcStageMask) ||
           IsFramebufferSpaceStage(selfDependency.dstStageMask));
    dependencies.push_back(selfDependency);
  }
  return dependencies;
}

} // namespace

namespace RHI::vulkan::utils
{

bool RenderPassBuilder::SetAttachments(std::span<const VkAttachmentDescription> attachments)
{
  if (!std::ranges::equal(m_attachments, attachments))
  {
    m_attachments.assign(attachments.begin(), attachments.end());
    m_attachmentsUsageTable.clear();
    return true;
  }
  return false;
}

void RenderPassBuilder::SetSubpasses(std::vector<VkSubpassDescription> && subpasses,
                                     std::vector<uint32_t> && selfDependencies)
{
  m_subpassDescriptions = std::move(subpasses);
  m_attachmentsUsageTable = BuildAttachmentUsageTable(m_subpassDescriptions, m_attachments);
  m_dependenciesGraph = BuildDependenciesGraph(m_attachmentsUsageTable, m_attachments.size(),
                                               m_subpassDescriptions, selfDependencies);
}

VkRenderPass RenderPassBuilder::Make(const VkDevice & device) const
{
  if (m_subpassDescriptions.empty() || m_attachments.empty())
    return VK_NULL_HANDLE;

  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
  renderPassCreateInfo.pAttachments = m_attachments.data();
  renderPassCreateInfo.subpassCount = static_cast<uint32_t>(m_subpassDescriptions.size());
  renderPassCreateInfo.pSubpasses = m_subpassDescriptions.data();
  renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(m_dependenciesGraph.size());
  renderPassCreateInfo.pDependencies = m_dependenciesGraph.data();

  if (auto res = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create render pass");

  return renderPass;
}

void RenderPassBuilder::Reset()
{
  m_attachments.clear();
  m_subpassDescriptions.clear();
  m_dependenciesGraph.clear();
  m_attachmentsUsageTable.clear();
}

BarrierInfo RenderPassBuilder::GetFinalLayoutForAttachment(uint32_t subpassIndex,
                                                           uint32_t attachmentIndex) const
{
    subpassIndex++;
  if (subpassIndex >= 0 && subpassIndex < m_subpassDescriptions.size() && attachmentIndex >= 0 &&
      attachmentIndex < m_attachments.size())
    return m_attachmentsUsageTable[subpassIndex * m_attachments.size() + attachmentIndex];
  else
    return BarrierInfo{VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};
}

std::span<const VkAttachmentDescription> RenderPassBuilder::GetCachedAttachments() const noexcept
{
  return m_attachments;
}
} // namespace RHI::vulkan::utils
