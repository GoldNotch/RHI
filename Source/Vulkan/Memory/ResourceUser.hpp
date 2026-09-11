#pragma once
#include <variant>

#include <Memory/BufferInterface.hpp>
#include <Memory/TextureInterface.hpp>

namespace RHI::vulkan
{
using ResourcePtr = std::variant<IInternalBuffer *, IInternalTexture *>;
using ResourceUsageInfo =
  std::tuple<ResourcePtr, VkPipelineStageFlags2, VkAccessFlags2, VkImageLayout>;

enum class SynchronizationFilter : uint8_t
{
  None = 0,
  BufferOnly = 1,
  ImageOnly = 2,
  All = 3,
};

inline bool FilterSatisfied(SynchronizationFilter f1, SynchronizationFilter f2) noexcept
{
  return (static_cast<uint8_t>(f1) & static_cast<uint8_t>(f2)) != 0;
}

struct IResourceUser
{
  virtual ~IResourceUser() = default;
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const = 0;
  virtual void SynchroniseResources(SynchronizationFilter filter,
                                    details::CommandBuffer & commands) const = 0;
};
} // namespace RHI::vulkan
