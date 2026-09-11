#include <vector>

#include <RHI.hpp>
#include <TestUtils.hpp>

// clang-format off
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
// clang-format on
extern RHI::ITexture * UploadTexture(const char *, RHI::IContext *, bool, bool);
int main()
{
  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  auto * texture = UploadTexture("mike_wazowski.jpg", ctx.get(), false);
  auto extent = texture->GetDescription().extent;
  std::vector<uint8_t> pixelData;
  pixelData.resize(extent[0] * extent[1] * 3);
  RHI::DownloadImageArgs args{};
  args.dstTexture.format = RHI::HostImageFormat::RGB8;
  args.dstTexture.type = RHI::ImageType::Image2D;
  args.dstTexture.extent = extent;
  args.dstTexture.pixelData = pixelData.data();
  args.copyRegion = {{0, 0, 0}, extent};
  auto task = texture->DownloadImage(args);
  ctx->TransferPass(); // call with true to future will complete
  task->Wait();
  stbi_write_bmp("downloaded_image.bmp", args.copyRegion.extent[0], args.copyRegion.extent[1], 3,
                 pixelData.data());
  return 0;
}
