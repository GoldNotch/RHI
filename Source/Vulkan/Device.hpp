#pragma once
#include <array>
#include <unordered_set>

#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
enum QueueType : uint8_t
{
  Present,
  Graphics,
  Compute,
  Transfer,
  Total
};

struct Device final : public OwnedBy<Context>
{
  explicit Device(Context & ctx, const GpuTraits & gpuTraits);
  ~Device() override;

  VkDevice GetDevice() const noexcept;
  VkInstance GetInstance() const noexcept;
  VkPhysicalDevice GetGPU() const noexcept;
  const VkPhysicalDeviceProperties & GetGpuProperties() const & noexcept;
  std::pair<uint32_t, VkQueue> GetQueue(QueueType type) const;
  uint32_t GetVulkanVersion() const noexcept;
  bool CheckExtension(std::string_view extension) const noexcept;

private:
  std::array<uint8_t, 9216> m_privateData; ///< private data. You can change size if it doesn't compile
  std::array<std::pair<uint32_t, VkQueue>, QueueType::Total> m_queues;
  std::unordered_set<std::string> m_availableExtensions;
};

} // namespace RHI::vulkan
