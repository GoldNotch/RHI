#pragma once

#include <span>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <Utils/DescriptorSetLayoutBuilder.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct Pipeline;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorBufferLayout final : public OwnedBy<Context>
{
  explicit DescriptorBufferLayout(Context & ctx);
  ~DescriptorBufferLayout();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  std::pair<VkDescriptorPool, std::vector<VkDescriptorSet>> AllocDescriptorSets() const;

  size_t GetLayoutsHash() const noexcept;
  std::span<const VkDescriptorSetLayout> GetLayouts() const noexcept;

  void DeclareDescriptorsArray(const LayoutIndex & index, VkDescriptorType type,
                               ShaderType shaderStage, uint32_t size);

  void Invalidate();

private:
  size_t m_layoutsHash = 0;
  std::vector<utils::DescriptorSetLayoutBuilder> m_builders; // for each set
  std::vector<VkDescriptorSetLayout> m_layouts;
  std::vector<VkDescriptorPoolSize> m_poolSizes; // for each VkDescriptorType
};

} // namespace RHI::vulkan
