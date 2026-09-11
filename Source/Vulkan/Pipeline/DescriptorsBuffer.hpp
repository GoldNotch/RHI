#pragma once

#include <mutex>
#include <variant>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Pipeline/UpdateDescriptorTask.hpp>
#include <Private/OwnedBy.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct DescriptorBufferLayout;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct DescriptorBuffer final : public RHI::OwnedBy<Context>
{
  explicit DescriptorBuffer(Context & ctx);
  virtual ~DescriptorBuffer() override;
  DescriptorBuffer(DescriptorBuffer && rhs) noexcept;
  DescriptorBuffer & operator=(DescriptorBuffer && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

  void Invalidate(const DescriptorBufferLayout & layout);

  std::span<const VkDescriptorSet> GetSets() const noexcept;

private:
  size_t m_cachedLayoutsHash = 0;
  VkDescriptorPool m_pool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_sets;
};

} // namespace RHI::vulkan
