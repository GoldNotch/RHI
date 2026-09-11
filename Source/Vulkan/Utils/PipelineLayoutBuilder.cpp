#include "PipelineLayoutBuilder.hpp"

#include <Utils/CastHelper.hpp>

namespace RHI::vulkan::utils
{
VkPipelineLayout PipelineLayoutBuilder::Make(const VkDevice & device,
                                             std::span<const VkDescriptorSetLayout> layouts) const
{
  // create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());           // Optional
  pipelineLayoutInfo.pSetLayouts = layouts.data();                                     // Optional
  pipelineLayoutInfo.pushConstantRangeCount = m_pushConstantRange.has_value() ? 1 : 0; // Optional
  pipelineLayoutInfo.pPushConstantRanges =
    m_pushConstantRange.has_value() ? &m_pushConstantRange.value() : nullptr; // Optional

  VkPipelineLayout layout;
  if (auto res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout);
      res != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout - ");
  return layout;
}

void PipelineLayoutBuilder::Reset()
{
  m_pushConstantRange = std::nullopt;
}

void PipelineLayoutBuilder::DeclarePushConstant(uint32_t size, ShaderType shaderStage)
{
  VkPushConstantRange newPushConstantRange{};
  newPushConstantRange.offset = 0;
  newPushConstantRange.size = size;
  newPushConstantRange.stageFlags =
    utils::CastInterfaceEnum2Vulkan<VkShaderStageFlagBits>(shaderStage);
  m_pushConstantRange.emplace(newPushConstantRange);
}

} // namespace RHI::vulkan::utils
