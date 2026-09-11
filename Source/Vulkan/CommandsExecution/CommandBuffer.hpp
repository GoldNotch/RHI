#pragma once
#include <numeric>

#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}


namespace RHI::vulkan::details
{
struct CommandBuffer : public OwnedBy<Context>
{
  explicit CommandBuffer(Context & ctx, uint32_t queue_family, VkCommandBufferLevel level);
  virtual ~CommandBuffer() override;
  CommandBuffer(CommandBuffer && rhs) noexcept;
  CommandBuffer & operator=(CommandBuffer && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

  void BeginWriting() const;
  void BeginWriting(VkRenderPass renderPass, uint32_t subpassIndex,
                    VkFramebuffer framebuffer = VK_NULL_HANDLE) const;
  void EndWriting() const;
  virtual void Reset();
  void AddCommands(std::span<const VkCommandBuffer> buffers);
  void AddCommands(VkCommandBuffer buffer);

  template<typename VkCmdFunc, typename... Args>
  void PushCommand(VkCmdFunc && func, Args &&... args) noexcept
  {
    func(m_buffer, std::forward<Args>(args)...);
    m_commandsCount++;
  }

  bool IsEmpty() const noexcept { return m_commandsCount == 0; }
  uint32_t GetBoundQueueFamily() const noexcept;

public:
  VkCommandBuffer GetHandle() const noexcept { return m_buffer; }

private:
  VkCommandBufferLevel m_level;
  uint32_t m_queueFamily = 0;
  VkCommandPool m_pool = VK_NULL_HANDLE;
  VkCommandBuffer m_buffer = VK_NULL_HANDLE;
  size_t m_commandsCount = 0;
};

} // namespace RHI::vulkan::details

namespace RHI::vulkan
{
struct ICommandWriter
{
  virtual ~ICommandWriter() = default;
  virtual void RecordCommands(details::CommandBuffer & commands) = 0;
};
} // namespace RHI::vulkan
