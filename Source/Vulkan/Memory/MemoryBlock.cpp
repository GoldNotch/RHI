#include "MemoryBlock.hpp"

#include <ImageUtils/ImageUtils.hpp>
#include <Utils/CastHelper.hpp>
#include <vk_mem_alloc.h>

#include "MemoryAllocator.hpp"

namespace
{
template<typename VkUsageFlagsT>
constexpr VmaAllocationCreateFlags CalcAllocationFlags(VkUsageFlagsT usage,
                                                       bool allowHostAccess) noexcept = delete;


template<>
constexpr VmaAllocationCreateFlags CalcAllocationFlags<VkBufferUsageFlagBits>(
  VkBufferUsageFlagBits usage, bool allowHostAccess) noexcept
{
  VmaAllocationCreateFlags flags = 0;
  const bool isStaging = usage ==
                         (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  // Staging buffers for transfers and uniforms (typically host-visible)
  if (isStaging || usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
  {
    flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  }

  // Device-local memory for buffers that GPU reads frequently
  if (usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT))
  {
    if (allowHostAccess)
      flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    else
      flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  }

  assert(flags != 0);
  return flags;
}


template<>
constexpr VmaAllocationCreateFlags CalcAllocationFlags<VkImageUsageFlagBits>(
  VkImageUsageFlagBits usage, bool allowHostAccess) noexcept
{
  return VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
}

} // namespace

namespace RHI::vulkan::memory
{

MemoryBlock::MemoryBlock(MemoryAllocator & allocator, const TextureDescription & description,
                         VkImageUsageFlags usage, VkSampleCountFlagBits samples,
                         VkSharingMode shareMode)
  : OwnedBy<MemoryAllocator>(allocator)
{
  auto [extent, layersCount] = utils::UnpackExtentAndLayers(description.extent, description.type);

  VkImageCreateInfo imageInfo{};
  {
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = utils::CastInterfaceEnum2Vulkan<VkImageType>(description.type);
    imageInfo.extent = extent;
    imageInfo.mipLevels = description.mipLevels;
    imageInfo.arrayLayers = layersCount;
    imageInfo.format = utils::CastInterfaceEnum2Vulkan<VkFormat>(description.format);
    imageInfo.tiling = /*description.format == RHI::ImageFormat::DEPTH_STENCIL
                       ? VK_IMAGE_TILING_LINEAR
                       :*/
      VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage == 0 ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : usage;
    imageInfo.samples = samples;
    imageInfo.sharingMode = shareMode;
  }
  VmaAllocationCreateFlags allocFlags =
    CalcAllocationFlags(static_cast<VkImageUsageFlagBits>(usage), false);
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags = allocFlags;
  allocCreateInfo.priority = 1.0f;

  VkImage image;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
  auto * allocatorHandle = reinterpret_cast<VmaAllocator>(GetAllocator().GetHandle());
  if (auto res = vmaCreateImage(allocatorHandle, &imageInfo, &allocCreateInfo, &image, &allocation,
                                &allocInfo);
      res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create VkImage");

  m_image = image;
  m_memBlock = allocation;
  m_allocInfo = reinterpret_cast<AllocInfoRawMemory &>(allocInfo);
  m_flags = allocFlags;
  m_size = allocInfo.size;
}

MemoryBlock::MemoryBlock(MemoryAllocator & allocator, size_t size, VkBufferUsageFlags usage,
                         bool allowHostAccess)
  : OwnedBy<MemoryAllocator>(allocator)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;

  VmaAllocationCreateFlags allocationFlags =
    CalcAllocationFlags(static_cast<VkBufferUsageFlagBits>(usage), allowHostAccess);
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags = allocationFlags;
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo;
  auto * allocatorHandle = reinterpret_cast<VmaAllocator>(GetAllocator().GetHandle());

  if (auto res = vmaCreateBuffer(allocatorHandle, &bufferInfo, &allocCreateInfo, &buffer,
                                 &allocation, &allocInfo);
      res != VK_SUCCESS)
    throw std::invalid_argument("Failed to create Buffer");

  m_buffer = buffer;
  m_memBlock = allocation;
  m_allocInfo = reinterpret_cast<AllocInfoRawMemory &>(allocInfo);
  m_size = std::min(allocInfo.size, size); // MemoryAllocator can alloc more then needed
  m_flags = allocationFlags;
}

MemoryBlock::MemoryBlock(MemoryBlock && rhs) noexcept
  : OwnedBy<MemoryAllocator>(std::move(rhs))
{
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_image, rhs.m_image);
  std::swap(m_allocInfo, rhs.m_allocInfo);
  std::swap(m_flags, rhs.m_flags);
  std::swap(m_memBlock, rhs.m_memBlock);
  std::swap(m_size, rhs.m_size);
}

MemoryBlock & MemoryBlock::operator=(MemoryBlock && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<MemoryAllocator>::operator=(std::move(rhs));
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_image, rhs.m_image);
    std::swap(m_allocInfo, rhs.m_allocInfo);
    std::swap(m_flags, rhs.m_flags);
    std::swap(m_memBlock, rhs.m_memBlock);
    std::swap(m_size, rhs.m_size);
  }
  return *this;
}

MemoryBlock::operator bool() const noexcept
{
  return !!m_image || !!m_buffer;
}

MemoryBlock::~MemoryBlock()
{
  auto * allocatorHandle = reinterpret_cast<VmaAllocator>(GetAllocator().GetHandle());
  if (m_buffer)
    vmaDestroyBuffer(allocatorHandle, m_buffer, reinterpret_cast<VmaAllocation>(m_memBlock));
  if (m_image)
    vmaDestroyImage(allocatorHandle, m_image, reinterpret_cast<VmaAllocation>(m_memBlock));
}

bool MemoryBlock::UploadSync(const void * data, size_t size, size_t offset)
{
  auto allocator = reinterpret_cast<VmaAllocator>(GetAllocator().GetHandle());
  auto allocation = reinterpret_cast<VmaAllocation>(m_memBlock);
  return vmaCopyMemoryToAllocation(allocator, data, allocation, offset, size) == VK_SUCCESS;
}

bool MemoryBlock::DownloadSync(size_t offset, void * data, size_t size) const
{
  auto allocator = reinterpret_cast<VmaAllocator>(GetAllocator().GetHandle());
  auto allocation = reinterpret_cast<VmaAllocation>(m_memBlock);
  return vmaCopyAllocationToMemory(allocator, allocation, offset, data, size) == VK_SUCCESS;
}

IBufferGPU::ScopedPointer MemoryBlock::Map()
{
  void * mapped_memory = nullptr;
  const bool is_mapped = IsMapped();
  auto allocator = reinterpret_cast<VmaAllocator>(GetAllocator().GetHandle());
  if (!is_mapped)
  {
    if (VkResult res =
          vmaMapMemory(allocator, reinterpret_cast<VmaAllocation>(m_memBlock), &mapped_memory);
        res != VK_SUCCESS)
      throw std::runtime_error("Failed to map buffer memory");
  }
  else
  {
    mapped_memory = reinterpret_cast<const VmaAllocationInfo &>(m_allocInfo).pMappedData;
  }

  return IBufferGPU::ScopedPointer(reinterpret_cast<char *>(mapped_memory),
                                   [this, is_mapped, allocator](char * mem_ptr)
                                   {
                                     if (!is_mapped && mem_ptr != nullptr)
                                       vmaUnmapMemory(allocator,
                                                      reinterpret_cast<VmaAllocation>(m_memBlock));
                                   });
}

void MemoryBlock::Flush() const noexcept
{
  auto allocator = reinterpret_cast<VmaAllocator>(GetAllocator().GetHandle());
  vmaFlushAllocation(allocator, reinterpret_cast<VmaAllocation>(m_memBlock), 0, m_size);
}

bool MemoryBlock::IsMapped() const noexcept
{
  return (m_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;
}

} // namespace RHI::vulkan::memory
