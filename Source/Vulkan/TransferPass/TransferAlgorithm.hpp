#pragma once
#include <Memory/BufferInterface.hpp>
#include <Memory/TextureInterface.hpp>
#include <RHI.hpp>
#include <TransferPass/TransferTask.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan::details
{

TrasferTaskPtr UploadBuffer(Context & ctx, IInternalBuffer & dstBuffer, const uint8_t * srcData,
                            size_t size, size_t offset);

TrasferTaskPtr DownloadBuffer(Context & ctx, IInternalBuffer & srcBuffer, uint8_t * dst,
                              size_t size, size_t offset);

TrasferTaskPtr UploadImage(Context & ctx, IInternalTexture & dstImage,
                           const UploadImageArgs & args);

TrasferTaskPtr DownloadImage(Context & ctx, IInternalTexture & srcImage,
                             const DownloadImageArgs & args);

TrasferTaskPtr GenerateMipmaps(Context & ctx, IInternalTexture & dst);

TrasferTaskPtr GenerateMipmapsByRegions(Context & ctx, IInternalTexture & dst,
                                        std::span<const RHI::TextureRegion> regions);

TrasferTaskPtr BlitImageToImage(Context & ctx, IInternalTexture & dst, IInternalTexture & src,
                                const TextureRegion & region);

} // namespace RHI::vulkan::details
