#pragma once

#include "VulkanContext.hpp"
namespace RHI::vulkan
{
namespace details
{
struct DescriptorSetLayoutBuilder;
}

struct DescriptorBuffer final
{
  explicit DescriptorBuffer(const Context & ctx);
  ~DescriptorBuffer();

  void Invalidate();

  IBufferGPU * DeclareUniform(uint32_t binding, ShaderType shaderStage, uint32_t size);

  void Bind(const vk::CommandBuffer & buffer, vk::PipelineLayout pipelineLayout,
            VkPipelineBindPoint bindPoint);
  vk::DescriptorSetLayout GetLayoutHandle() const noexcept;
  vk::DescriptorSet GetHandle() const noexcept;
  using CapacityDistribution = std::unordered_map<VkDescriptorType, uint32_t>;

private:
  const Context & m_owner;
  CapacityDistribution m_capacity;

  vk::DescriptorSetLayout m_layout = VK_NULL_HANDLE;
  vk::DescriptorSet m_set = VK_NULL_HANDLE;
  vk::DescriptorPool m_pool = VK_NULL_HANDLE;
  using DescriptorBufferGPU = std::pair<VkDescriptorType, std::unique_ptr<IBufferGPU>>;
  std::vector<DescriptorBufferGPU> m_buffers; //< buffer for each binding slot

  std::unique_ptr<details::DescriptorSetLayoutBuilder> m_layoutBuilder;

  bool m_invalidLayout : 1 = false;
  bool m_invalidPool : 1 = false;
  bool m_invalidSet : 1 = false;
};

} // namespace RHI::vulkan
