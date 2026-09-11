#pragma once

#include <Memory/ResourceUser.hpp>
#include <Pipeline/UpdateDescriptorTask.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>


namespace RHI::vulkan
{
struct Context;
struct Pipeline;
} // namespace RHI::vulkan

namespace RHI::vulkan::details
{
struct CommandBuffer;


struct BaseDescriptor : public OwnedBy<Context>, //TODO: remove
                        public OwnedBy<Pipeline>,
                        public IResourceUser
{
  explicit BaseDescriptor(Context & ctx, Pipeline & pipeline, VkDescriptorType type,
                          LayoutIndex index, uint32_t arrayIndex = 0)
    : OwnedBy<Context>(ctx)
    , OwnedBy<Pipeline>(pipeline)
    , m_type(type)
    , m_arrayIndex(arrayIndex)
    , m_index(index)
  {
  }
  virtual ~BaseDescriptor() override = default;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(Pipeline, GetPipeline);

  BaseDescriptor(BaseDescriptor && rhs) noexcept
    : OwnedBy<Context>(std::move(rhs))
    , OwnedBy<Pipeline>(std::move(rhs))
  {
    std::swap(m_type, rhs.m_type);
    std::swap(m_arrayIndex, rhs.m_arrayIndex);
    std::swap(m_index, rhs.m_index);
  }

  BaseDescriptor & operator=(BaseDescriptor && rhs) noexcept
  {
    if (this != &rhs)
    {
      OwnedBy<Context>::operator=(std::move(rhs));
      OwnedBy<Pipeline>::operator=(std::move(rhs));
      std::swap(m_type, rhs.m_type);
      std::swap(m_arrayIndex, rhs.m_arrayIndex);
      std::swap(m_index, rhs.m_index);
    }
    return *this;
  }

  VkDescriptorType GetDescriptorType() const noexcept { return m_type; }
  uint32_t GetArrayIndex() const noexcept { return m_arrayIndex; }
  uint32_t GetBinding() const noexcept { return m_index.binding; }
  uint32_t GetSet() const noexcept { return m_index.set; }
  virtual void Invalidate() = 0;
  virtual void UpdateDescriptorSet(std::span<const VkDescriptorSet> sets) const = 0;

protected:
  VkDescriptorType m_type;
  uint32_t m_arrayIndex = 0;
  LayoutIndex m_index{0, 0};
};


} // namespace RHI::vulkan::details
