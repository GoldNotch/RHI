#pragma once
#include <Memory/MemoryBlock.hpp>
#include <Private/OwnedBy.hpp>
#include <Memory/BufferInterface.hpp>
#include <Memory/Synchronizer.hpp>
#include <RHI.hpp>

namespace RHI::vulkan
{
struct Context;
} // namespace RHI::vulkan

namespace RHI::vulkan
{

struct BufferGPU : public IBufferGPU,
                   public IInternalBuffer,
                   public OwnedBy<Context>
{
  using IBufferGPU::ScopedPointer;

  explicit BufferGPU(Context & ctx, size_t size, BufferGPUUsage usage, bool allowHostAccess);
  explicit BufferGPU(Context & ctx, size_t size, VkBufferUsageFlags usage, bool allowHostAccess);
  virtual ~BufferGPU() override;
  BufferGPU(BufferGPU && rhs) noexcept;
  BufferGPU & operator=(BufferGPU && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  RESTRICTED_COPY(BufferGPU);

public: //IBufferGPU interface
  virtual void UploadSync(const void * data, size_t size, size_t offset = 0) override;
  virtual std::shared_ptr<IAwaitable> UploadAsync(const void * data, size_t size,
                                                size_t offset = 0) override;
  virtual ScopedPointer Map() override;
  virtual void Flush() const noexcept override;
  virtual bool IsMapped() const noexcept override;
  virtual size_t Size() const noexcept override;

public: //IInternalBuffer interface
  virtual VkBuffer GetHandle() const noexcept override;
  virtual size_t GetSize() const noexcept override;
  virtual details::Synchronizer & GetSynchronizer() & noexcept override;

private:
  memory::MemoryBlock m_memBlock;
  details::Synchronizer m_synchronizer;
};
} // namespace RHI::vulkan
