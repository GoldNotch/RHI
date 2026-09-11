#pragma once
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct DescriptorSetLayoutBuilder final
{
  VkDescriptorSetLayout Make(const VkDevice & device) const;
  void Reset();
  void DeclareDescriptor(uint32_t binding, VkDescriptorType type, ShaderType shaderStage);
  void DeclareDescriptorsArray(uint32_t binding, VkDescriptorType type, ShaderType shaderStage,
                               uint32_t size);

private:
  std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};
} // namespace RHI::vulkan::utils
