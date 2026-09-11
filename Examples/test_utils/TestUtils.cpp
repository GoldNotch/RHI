#include "TestUtils.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>

#include <GLFW/glfw3.h>

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// clang-format on

#include <rectpack2D/finders_interface.h>


void ConsoleLog(RHI::LogMessageStatus status, const std::string & message)
{
  switch (status)
  {
    case RHI::LogMessageStatus::LOG_INFO:
      std::printf("INFO: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_WARNING:
      std::printf("WARNING: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_ERROR:
      std::printf("ERROR: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_DEBUG:
      std::printf("DEBUG: - %s\n", message.c_str());
      break;
  }
}

RHI::ITexture * UploadTexture(const char * path, RHI::IContext * ctx, bool with_alpha,
                              bool useMips /* = false*/)
{
  int w = 0, h = 0, channels = 3;
  uint8_t * pixel_data = stbi_load(path, &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
  if (!pixel_data)
    throw std::runtime_error("Failed to load texture. Check it exists near the exe file");

  RHI::HostTextureView hostTexture{};
  {
    hostTexture.extent = {static_cast<RHI::texel_t>(w), static_cast<RHI::texel_t>(h), 1};
    hostTexture.pixelData = reinterpret_cast<uint8_t *>(pixel_data);
    hostTexture.format = with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8;
    hostTexture.type = RHI::ImageType::Image2D;
  };

  RHI::TextureDescription imageArgs{};
  {
    imageArgs.extent = hostTexture.extent;
    imageArgs.type = RHI::ImageType::Image2D;
    imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
    imageArgs.mipLevels = useMips ? RHI::CalcMaxMipLevels(imageArgs.extent) : 1;
  }
  auto texture = ctx->CreateTexture(imageArgs);

  RHI::UploadImageArgs args{};
  {
    args.srcTexture = hostTexture;
    args.copyRegion = {{0, 0, 0}, hostTexture.extent};
  }
  texture->UploadImage(args);
  if (useMips)
    texture->GenerateMipmaps();
  stbi_image_free(pixel_data);
  return texture;
}

RHI::ITexture * UploadLayeredTexture(RHI::IContext * ctx,
                                     const std::vector<std::filesystem::path> & paths,
                                     bool with_alpha, bool useMips /* = false*/)
{
  int width = 0, height = 0;
  std::vector<RHI::HostTextureView> textures;
  for (auto && path : paths)
  {
    int w = 0, h = 0, channels = 3;
    uint8_t * pixel_data =
      stbi_load(path.string().c_str(), &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
    if (!pixel_data)
      throw std::runtime_error("Failed to load texture. Check it exists near the exe file");
    if (width != 0 && width != w || height != 0 && height != h)
      throw std::runtime_error("Images have different sizes");

    auto && hostTexture = textures.emplace_back();
    hostTexture.pixelData = pixel_data;
    hostTexture.extent = {static_cast<RHI::texel_t>(w), static_cast<RHI::texel_t>(h), 1};
    hostTexture.type = RHI::ImageType::Image2D;
    hostTexture.format = with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8;
  }

  bool allTheSameSize = std::all_of(textures.begin(), textures.end(), [&](auto && texture)
                                    { return texture.extent == textures.front().extent; });
  assert(allTheSameSize);

  RHI::TextureDescription imageArgs{};
  {
    imageArgs.extent = textures.front().extent;
    imageArgs.extent[2] = static_cast<uint32_t>(textures.size());
    imageArgs.type = RHI::ImageType::Image2D_Array;
    imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
    imageArgs.mipLevels = useMips ? RHI::CalcMaxMipLevels(imageArgs.extent) : 1;
  }
  auto texture = ctx->CreateTexture(imageArgs);

  for (uint32_t i = 0; i < textures.size(); ++i)
  {
    RHI::HostTextureView & hostTexture = textures[i];
    RHI::UploadImageArgs args{};
    {
      args.srcTexture = hostTexture;
      args.copyRegion = {{0, 0, 0}, {hostTexture.extent}};
      args.dstOffset = {0, 0, static_cast<RHI::texel_t>(i)};
    }
    texture->UploadImage(args);
  }

  if (useMips)
    texture->GenerateMipmaps();

  for (auto && hostTexture : textures)
    stbi_image_free(hostTexture.pixelData);
  textures.clear();
  return texture;
}

RHI::ITexture * BuildAtlasTexture(RHI::IContext * ctx, uint16_t atlasSize,
                                  const std::vector<std::filesystem::path> & paths, bool with_alpha,
                                  bool useMips)
{
  using namespace rectpack2D;
  using spaces_type = rectpack2D::empty_spaces<false, default_empty_spaces>;
  using rect_t = output_rect_t<spaces_type>;
  auto report_successful = [](rect_t &)
  {
    return callback_result::CONTINUE_PACKING;
  };
  auto report_unsuccessful = [](rect_t &)
  {
    return callback_result::ABORT_PACKING;
  };


  int width = 0, height = 0;
  std::vector<RHI::HostTextureView> textures;
  std::vector<rect_t> rects;
  for (auto && path : paths)
  {
    int w = 0, h = 0, channels = 3;
    uint8_t * pixel_data =
      stbi_load(path.string().c_str(), &w, &h, &channels, with_alpha ? STBI_rgb_alpha : STBI_rgb);
    if (!pixel_data)
      throw std::runtime_error("Failed to load texture. Check it exists near the exe file");

    auto && hostTexture = textures.emplace_back();
    hostTexture.pixelData = pixel_data;
    hostTexture.extent = {static_cast<RHI::texel_t>(w), static_cast<RHI::texel_t>(h), 1};
    hostTexture.type = RHI::ImageType::Image2D;
    hostTexture.format = with_alpha ? RHI::HostImageFormat::RGBA8 : RHI::HostImageFormat::RGB8;

    rects.push_back(rect_xywh(0, 0, w, h));
  }

  RHI::TextureDescription imageArgs{};
  {
    imageArgs.extent = {atlasSize, atlasSize, 1};
    imageArgs.type = RHI::ImageType::Image2D;
    imageArgs.format = with_alpha ? RHI::ImageFormat::RGBA8 : RHI::ImageFormat::RGB8;
    imageArgs.mipLevels = useMips ? RHI::CalcMaxMipLevels(imageArgs.extent) : 1;
  }
  auto texture = ctx->CreateTexture(imageArgs);

  const auto result_size =
    find_best_packing<spaces_type>(rects, make_finder_input(atlasSize, -4 /*discard_step */,
                                                            report_successful, report_unsuccessful,
                                                            flipping_option::DISABLED));

  std::vector<RHI::TextureRegion> mipRegions;
  mipRegions.resize(textures.size());
  for (size_t i = 0; i < textures.size(); ++i)
  {
    auto && hostTexture = textures[i];
    RHI::UploadImageArgs args{};
    {
      args.srcTexture = hostTexture;
      args.copyRegion = {{0, 0, 0}, {hostTexture.extent}};
      args.dstOffset = {static_cast<uint16_t>(rects[i].x), static_cast<uint16_t>(rects[i].y), 0};
    }
    texture->UploadImage(args);
    mipRegions[i].offset = {static_cast<uint16_t>(rects[i].x), static_cast<uint16_t>(rects[i].y),
                            0};
    mipRegions[i].extent = {static_cast<uint16_t>(rects[i].w), static_cast<uint16_t>(rects[i].h),
                            1};
  }

  if (useMips)
    texture->GenerateMipmaps();

  for (auto && hostTexture : textures)
    stbi_image_free(hostTexture.pixelData);
  textures.clear();
  return texture;
}

RHI::SpirV ReadSpirV(const std::filesystem::path & path)
{
  // Open file at end to get size immediately
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Failed to open SPIR-V file: ");

  // Get size in bytes
  size_t fileSize = static_cast<size_t>(file.tellg());
  if (fileSize % 4 != 0)
    throw std::runtime_error("SPIR-V file size is not a multiple of 4 bytes: ");

  // Allocate vector with proper number of 32-bit words
  RHI::SpirV buffer(fileSize / 4, 0);

  // Seek back to start and read data
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), fileSize);

  if (!file)
    throw std::runtime_error("Failed to read complete SPIR-V file");

  return buffer;
}

std::filesystem::path FromGLSL(const std::filesystem::path & path)
{
  constexpr const char * apiFolder = ".vulkan";
  constexpr const char * spirvExtension = ".spv";

  std::filesystem::path suffix;
  auto && ext = path.extension();
  if (ext == ".vert")
    suffix = "_vert";
  else if (ext == ".frag")
    suffix = "_frag";
  else if (ext == ".geom")
    suffix = "_geom";
  else if (ext == ".glsl")
  {
    suffix = "";
  }
  else
    assert(false && "Invalid format for shader file");
  std::filesystem::path result = path.parent_path() / path.stem();
  result += suffix;
  result += spirvExtension;
  return result;
}

namespace RHI::test_examples
{

GlfwInstance::GlfwInstance()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

GlfwInstance::~GlfwInstance()
{
  glfwTerminate();
}

} // namespace RHI::test_examples
