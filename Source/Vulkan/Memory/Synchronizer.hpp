#pragma once
#include <mutex>

#include <Private/OwnedBy.hpp>
#include <vulkan/vulkan.hpp>


namespace RHI::vulkan::details
{
struct CommandBuffer;
}
namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
struct BarrierInfo final
{
  VkPipelineStageFlags2 currentStage = VK_PIPELINE_STAGE_2_NONE;
  VkAccessFlagBits2 requiredAccess = VK_ACCESS_2_NONE;
  VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};
} // namespace RHI::vulkan

namespace RHI::vulkan::details
{
struct Synchronizer final : public OwnedBy<Context>
{
  explicit Synchronizer(Context & ctx, VkImage image);
  explicit Synchronizer(Context & ctx, VkBuffer buffer);
  Synchronizer(Synchronizer && rhs) noexcept;
  Synchronizer & operator=(Synchronizer && rhs) noexcept;
  ~Synchronizer();
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  void ResetSynchronization();

  void RequireSynchronize(VkPipelineStageFlags2 currentStage, VkAccessFlagBits2 requiredAccess,
                          details::CommandBuffer & commands,
                          VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED);

  VkImageLayout GetLayout() const noexcept;
  /// @brief for external set of layout (f.e. in renderPass begin/end)
  void ExternalSynchronization(const BarrierInfo & barrier) noexcept;

private:
  VkImage m_image = VK_NULL_HANDLE;   ///< synchronizable image
  VkBuffer m_buffer = VK_NULL_HANDLE; ///< synchronizable buffer

  mutable std::mutex m_syncMutex;
  BarrierInfo m_prevBarrier;
};
} // namespace RHI::vulkan::details
