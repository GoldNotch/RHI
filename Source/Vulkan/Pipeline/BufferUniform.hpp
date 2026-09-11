#pragma once

#include <Memory/BufferInterface.hpp>
#include <Pipeline/BaseDescriptor.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>


namespace RHI::vulkan
{

struct BufferUniform final : public IBufferUniformDescriptor,
                             public details::BaseDescriptor
{
  explicit BufferUniform(Context & ctx, Pipeline & pipeline, VkDescriptorType type,
                         LayoutIndex index, uint32_t arrayIndex = 0);
  virtual ~BufferUniform() override = default;
  BufferUniform(BufferUniform && rhs) noexcept;
  BufferUniform & operator=(BufferUniform && rhs) noexcept;

public:
  virtual void UpdateDescriptorSet(std::span<const VkDescriptorSet> sets) const override;


public: // IBufferUniformDescriptor interface
  virtual void AssignBuffer(IBufferGPU * buffer, size_t offset = 0) override;
  virtual bool IsBufferAssigned() const noexcept override;

public: // IUniformDescriptor interface
  virtual uint32_t GetSet() const noexcept override { return BaseDescriptor::GetSet(); }
  virtual uint32_t GetBinding() const noexcept override { return BaseDescriptor::GetBinding(); }
  virtual uint32_t GetArrayIndex() const noexcept override
  {
    return BaseDescriptor::GetArrayIndex();
  }

public: // IResourceUser
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const override;
  virtual void SynchroniseResources(SynchronizationFilter filter, details::CommandBuffer & commands) const override;

public: // IInvalidable interface
  virtual void Invalidate() override;
  void SetInvalid();

public: // public internal API
  size_t GetOffset() const noexcept { return m_offset; }
  VkBuffer GetBuffer() const noexcept;
  using BaseDescriptor::GetDescriptorType;

private:
  mutable std::atomic_bool m_shouldUpdate = false;
  IInternalBuffer * m_buffer = nullptr;
  size_t m_offset = 0;
};

} // namespace RHI::vulkan
