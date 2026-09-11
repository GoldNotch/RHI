#pragma once
#include <RHI_def.h>

#include <array>
#include <cstdint>
#include <limits>

#include <Utils.hpp>

namespace RHI
{

/// @brief Defines image's layout in memory. Also defines if image can have mipmaps or not
///        For example image1d is line of pixels, image2d is a rectangle of pixels
enum class ImageType : uint8_t
{
  Image1D, ///< image with height = 1. has only width.Can have one mipmap for a whole image
  Image2D, ///< generic image with width and height. Can have one mipmap for a whole image
  Image3D, ///< layered image2d (with depth). Can have one mipmap for a whole image
  Image1D_Array, ///< array of 1d images, layered in memory like 2d image, but each row of pixels has its own mipmap
  Image2D_Array, ///< the same as image3d, but each layer should have own mipmap
  Cubemap,       ///< it's image2d_array with length = 6
};

/// @brief Defines samples count per pixel in image
enum class SamplesCount : uint8_t
{
  One = 1,
  Two = 2,
  Four = 4,
  Eight = 8,
};

/// @brief Defines in what format image will be uploaded or downloaded
enum class HostImageFormat : uint8_t
{
  R8,
  A8,
  RG8,
  BGR8,
  RGB8,
  RGBA8,
  BGRA8,
  // compressed types
  //BC1,
  //BC5,
  //BC7
};

/// @brief internal image format
// TODO: Remove
enum class ImageFormat : uint8_t
{
  UNDEFINED,
  // general formats
  R8,
  A8,
  RG8,
  BGR8,
  RGB8,
  RGBA8,
  BGRA8,
  // service formats
  DEPTH,
  DEPTH_STENCIL,
  // compressed types
  //BC1,
  //BC5,
  //BC7
};

/// @brief
enum ShaderAttachmentSlot : uint8_t
{
  Undefined = 0,                         ///< no attachment used in subpass
  Color = utils::bit<uint8_t>(1),        ///< color attachment
  DepthStencil = utils::bit<uint8_t>(2), ///< depth-stencil attachment
  Input = utils::bit<uint8_t>(4),        ///< read-only shader attachment
  Preserved = 0x80,                      ///< flag, that image just stored and unused in shader
  TOTAL = 4
};
DECLARE_BIT_OPERATIONS_FOR_ENUM(ShaderAttachmentSlot)

using texel_t = uint16_t;
/// For Image1D used only 0'th index
/// For Image2D used only 0 and 1 index
/// For Image3D used all 3 indices like width, height and layer
/// For Image1D used 0 and 1 index as width and i-th array element
/// For Image2D used all 3 indices as width, height and i-th array element
/// For Cubemap used all 3 indices as width, height and i-th surface of cube
using TexelIndex = std::array<texel_t, 3>;
static constexpr texel_t g_InvalidTexel = std::numeric_limits<texel_t>::max();

/// Gabarit of texture
using TextureExtent = TexelIndex;

struct TextureRegion final
{
  TexelIndex offset{0, 0, 0};
  TextureExtent extent{g_InvalidTexel, g_InvalidTexel, g_InvalidTexel};
  int reserved = 0;
};


struct TextureDescription final
{
  TextureExtent extent; ///< gabarit of texture (width, height, depth/layer)
  ImageType type;
  ImageFormat format;
  uint32_t mipLevels = 1;
};

RHI_API uint32_t CalcMaxMipLevels(TextureExtent extent, uint32_t minLength = 1);

struct HostTextureView final
{
  TextureExtent extent{g_InvalidTexel, g_InvalidTexel,
                       g_InvalidTexel}; ///< size of image (width, height, depth/layers)
  uint8_t * pixelData = nullptr;        ///< pointer on pixel data
  HostImageFormat format;               ///< format of pixel
  ImageType type;                       ///< type of image (1D, 2D, 3D, Cubemap, etc)
};

struct UploadImageArgs final
{
  HostTextureView srcTexture;    ///< host texture
  TextureRegion copyRegion{};    ///< subregion of host texture to copy
  TexelIndex dstOffset{0, 0, 0}; ///< offset in destination where to copy
};

struct DownloadImageArgs final
{
  HostTextureView dstTexture; ///< destination texture
  TextureRegion copyRegion{}; ///< where from copy texels
};

} // namespace RHI
