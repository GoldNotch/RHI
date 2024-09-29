#pragma once
#include "VulkanContext.hpp"

namespace RHI::vulkan
{
struct BuffersAllocator final
{
  explicit BuffersAllocator(const Context & ctx);
  ~BuffersAllocator();

  details::InternalHandle GetHandle() const noexcept { return m_allocator; }

private:
  details::InternalHandle m_allocator;
};


struct BufferGPU : public IBufferGPU
{
  using IBufferGPU::ScopedPointer;

  explicit BufferGPU(size_t size, BufferGPUUsage usage, BuffersAllocator & allocator,
                     bool mapped = false);
  virtual ~BufferGPU() override;

  virtual ScopedPointer Map() override;
  virtual void Flush() const noexcept override;
  virtual bool IsMapped() const noexcept override;
  virtual InternalObjectHandle GetHandle() const noexcept override;
  virtual size_t Size() const noexcept override { return m_size; }

private:
  using AllocInfoRawMemory = std::array<uint32_t, 14>;

  BuffersAllocator & m_allocator;

  AllocInfoRawMemory m_allocInfo;
  vk::Buffer m_buffer = nullptr;
  details::InternalHandle m_memBlock = nullptr;
  size_t m_size = 0;
  uint32_t m_flags = 0;
};


} // namespace RHI::vulkan
