#pragma once
#include "VulkanContext.hpp"

namespace RHI::vulkan
{
struct CommandBuffer final : public ICommandBuffer
{
  CommandBuffer() = default;
  explicit CommandBuffer(vk::Device device, vk::CommandPool pool, CommandBufferType type);

  void BeginWritingInSwapchain();
  virtual void BeginWriting(const IFramebuffer & framebuffer, const IPipeline & pipeline) override;
  virtual void EndWriting() override;

  virtual void DrawVertices(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0,
                            uint32_t firstInstance = 0) const override;
  virtual void DrawIndexedVertices(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                   uint32_t firstInstance = 0) const override;

  virtual void SetViewport(float width, float height) override;
  virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
  virtual void BindVertexBuffer(uint32_t binding, const IBufferGPU & buffer,
                                uint32_t offset = 0) override;
  virtual void BindIndexBuffer(const IBufferGPU & buffer, IndexType type,
                               uint32_t offset = 0) override;


  virtual void Reset() const override;
  virtual void AddCommands(const ICommandBuffer & buffer) const override;
  virtual CommandBufferType GetType() const noexcept override { return m_type; }


  vk::CommandBuffer GetHandle() const noexcept { return m_buffer; }

private:
  vk::CommandBuffer m_buffer = VK_NULL_HANDLE;
  RHI::CommandBufferType m_type;
};
} // namespace RHI::vulkan
