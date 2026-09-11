#pragma once
#include <atomic>
#include <functional>
#include <tuple>
#include <variant>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Memory/ResourceUser.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct Pipeline;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

/// @brief class to accumulate commands to call them every frame
struct PipelineProcess final : public RHI::IPipelineProcess,
                               public IResourceUser,
                               public OwnedBy<Context>
{
  explicit PipelineProcess(Context & ctx);
  virtual ~PipelineProcess() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // Commands
  /// @brief draw vertices command (analog glDrawArrays)
  virtual void DrawVertices(std::uint32_t vertexCount, std::uint32_t instanceCount,
                            std::uint32_t firstVertex = 0,
                            std::uint32_t firstInstance = 0) override;

  /// @brief draw vertices with indieces (analog glDrawElements)
  virtual void DrawIndexedVertices(std::uint32_t indexCount, std::uint32_t instanceCount,
                                   std::uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                   std::uint32_t firstInstance = 0) override;

  /// @brief Set viewport command
  virtual void SetViewport(float width, float height) override;

  /// @brief Set scissor command
  virtual void SetScissor(int32_t x, int32_t y, std::uint32_t width, std::uint32_t height) override;

  /// @brief binds buffer as input attribute data
  virtual void BindVertexBuffer(std::uint32_t binding, IBufferGPU * buffer,
                                std::uint32_t offset = 0) override;

  /// @brief binds buffer as index buffer
  virtual void BindIndexBuffer(IBufferGPU * buffer, IndexType type,
                               std::uint32_t offset = 0) override;

  virtual void PushConstant(const void * data, size_t size) override;

public: // IResourceUser
  /// @brief return true if process uses vkCmdPipelineBarrier.
  /// used to detect if RenderPass should have self-dependency
  bool RequireSynchronization() const;
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const override;
  virtual void SynchroniseResources(SynchronizationFilter filter,
                                    details::CommandBuffer & commands) const override;

public:
  void RecordCommands(details::CommandBuffer & commands, const Pipeline & pipeline);

private:
  using DrawCommand = std::function<void(details::CommandBuffer &, const Pipeline &)>;
  std::vector<DrawCommand> m_commands;
  std::vector<ResourceUsageInfo> m_resourceSyncInfos;
};

} // namespace RHI::vulkan
