#include "DescriptorBufferLayout.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

#include <Pipeline/Pipeline.hpp>
#include <Private/Hash.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan::details
{
VkDescriptorPool CreateDescriptorPool(const Context & ctx,
                                      std::span<const VkDescriptorPoolSize> poolSizes)
{
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = std::accumulate(poolSizes.begin(), poolSizes.end(), 0u,
                                     [](uint32_t acc, const VkDescriptorPoolSize & pool_size)
                                     { return acc + pool_size.descriptorCount; });
  assert(poolInfo.maxSets != 0);

  VkDescriptorPool c_pool;
  if (vkCreateDescriptorPool(ctx.GetGpuConnection().GetDevice(), &poolInfo, nullptr, &c_pool) !=
      VK_SUCCESS)
    throw std::runtime_error("failed to create VkDescriptorPool!");
  return VkDescriptorPool(c_pool);
}

VkDescriptorSet CreateDescriptorSet(const Context & ctx, VkDescriptorPool pool,
                                    VkDescriptorSetLayout layout)
{
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
  if (vkAllocateDescriptorSets(ctx.GetGpuConnection().GetDevice(), &allocInfo, &descriptor_set) !=
      VK_SUCCESS)
    throw std::runtime_error("failed to allocate VkDescriptorSet!");
  return VkDescriptorSet(descriptor_set);
}


} // namespace RHI::vulkan::details

namespace RHI::vulkan
{

DescriptorBufferLayout::DescriptorBufferLayout(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

DescriptorBufferLayout::~DescriptorBufferLayout()
{
  for (auto layout : m_layouts)
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(layout, nullptr);
}

void DescriptorBufferLayout::Invalidate()
{
  if (m_layoutsHash == 0)
  {
    assert(m_builders.size() == m_layouts.size());

    for (size_t i = 0, c = m_builders.size(); i < c; ++i)
    {
      VkDescriptorSetLayout newLayout =
        m_builders[i].Make(GetContext().GetGpuConnection().GetDevice());
      GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_layouts[i], nullptr);
      GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG,
                       "VkDescriptorSetLayout({}) has been rebuilt - {}",
                       static_cast<void *>(m_layouts[i]), static_cast<void *>(newLayout));
      m_layouts[i] = newLayout;
      RHI::utils::HashCombine(m_layoutsHash, newLayout);
    }
  }
}

std::pair<VkDescriptorPool, std::vector<VkDescriptorSet>> DescriptorBufferLayout::
  AllocDescriptorSets() const
{
  auto pool = details::CreateDescriptorPool(GetContext(), m_poolSizes);
  std::vector<VkDescriptorSet> sets;
  sets.reserve(m_layouts.size());
  for (auto && layout : m_layouts)
    sets.push_back(details::CreateDescriptorSet(GetContext(), pool, layout));
  return {pool, std::move(sets)};
}

size_t DescriptorBufferLayout::GetLayoutsHash() const noexcept
{
  return m_layoutsHash;
}

std::span<const VkDescriptorSetLayout> DescriptorBufferLayout::GetLayouts() const noexcept
{
  return m_layouts;
}

void DescriptorBufferLayout::DeclareDescriptorsArray(const LayoutIndex & index,
                                                     VkDescriptorType type, ShaderType shaderStage,
                                                     uint32_t size)
{
  const uint32_t setIdx = index.set;
  while (m_layouts.size() <= setIdx)
    m_layouts.push_back(VK_NULL_HANDLE);
  while (m_builders.size() <= setIdx)
    m_builders.emplace_back();
  m_builders[setIdx].DeclareDescriptorsArray(index.binding, type, shaderStage, size);
  m_layoutsHash = 0;

  auto it = std::ranges::find_if(m_poolSizes, [type](const VkDescriptorPoolSize & poolSize)
                                 { return poolSize.type == type; });
  if (it == m_poolSizes.end())
    m_poolSizes.push_back(VkDescriptorPoolSize{type, size});
  else
    it->descriptorCount += size;
}

} // namespace RHI::vulkan
