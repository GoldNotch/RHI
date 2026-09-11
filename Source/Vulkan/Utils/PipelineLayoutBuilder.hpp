#pragma once
#include <optional>
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct PipelineLayoutBuilder final
{
  VkPipelineLayout Make(const VkDevice & device,
                        std::span<const VkDescriptorSetLayout> layouts) const;
  void Reset();

  void DeclarePushConstant(uint32_t size, ShaderType shaderStage);

private:
  std::optional<VkPushConstantRange> m_pushConstantRange = std::nullopt;
};
} // namespace RHI::vulkan::utils
