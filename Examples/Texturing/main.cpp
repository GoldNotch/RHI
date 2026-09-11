#include <cmath>
#include <cstdio>
#include <cstring>

#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

struct PushConstant
{
  float scale_x;
  float scale_y;
  float pos_x;
  float pos_y;
  uint32_t texture_index;
};

int main()
{
  RHI::test_examples::GlfwInstance instance;
  RHI::test_examples::Window window("Texturing", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  std::vector<RHI::ITexture *> textures;
  textures.emplace_back(UploadTexture("texture.png", ctx.get(), true));
  textures.emplace_back(UploadTexture("jackal.jpg", ctx.get(), false));
  textures.emplace_back(UploadTexture("shrek1.jpg", ctx.get(), false));
  textures.emplace_back(UploadTexture("shrek2.jpg", ctx.get(), false));
  textures.emplace_back(UploadTexture("mike_wazowski.jpg", ctx.get(), false));
  textures.emplace_back(UploadTexture("pepe.jpg", ctx.get(), false));
  auto image_it = textures.begin();

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  auto * colorAttachment =
    ctx->CreateSurfacedAttachment(window.GetDrawSurface(), RHI::RenderBuffering::Triple);
  framebuffer->AddAttachment(0, colorAttachment);

  window.onResize = [framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };

  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto trianglePipeline = ctx->CreatePipeline();
  trianglePipeline->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  trianglePipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("textures.vert")));
  trianglePipeline->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("textures.frag")));
  trianglePipeline->DefinePushConstant(sizeof(PushConstant),
                                       RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);

  std::vector<RHI::ISamplerUniformDescriptor *> samplers;
  auto it = textures.begin();
  for (uint32_t i = 0; i < 8; ++i)
  {
    auto * texture = trianglePipeline->DeclareSampler({0, i}, RHI::ShaderType::Fragment);
    texture->SetFilter(RHI::TextureFilteration::Linear, RHI::TextureFilteration::Linear);
    texture->AssignImage(*it);
    samplers.push_back(texture);
    it = std::next(it);
    if (it == textures.end())
      it = textures.begin();
  }

  auto process = ctx->CreateProcess();
  {
    auto [width, height] = window.GetSize();
    process->SetViewport(static_cast<float>(width), static_cast<float>(height));
    process->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    constexpr int cells_count_in_row = 8;
    constexpr float cell_width = 2.0f / cells_count_in_row;
    constexpr float offset = -1.0f + cell_width / 2.0f;
    constexpr float margin = 0.05f;
    PushConstant ct{};
    ct.scale_x = cell_width / 2.0f;
    ct.scale_y = cell_width / 2.0f;
    ct.texture_index = 0;
    for (int i = 0; i <= cells_count_in_row; ++i)
    {
      for (int j = 0; j <= cells_count_in_row; ++j)
      {
        ct.pos_x = offset + j * (cell_width + margin);
        ct.pos_y = offset + i * (cell_width + margin);
        ct.texture_index = (ct.texture_index + 1) % textures.size();
        process->PushConstant(&ct, sizeof(PushConstant));
        process->DrawVertices(6, 1); 
      }
    } 
  }
  framebuffer->SetSubpass(0, trianglePipeline, process);

  colorAttachment->SetClearValue(0.3f, 0.3f, 0.5f, 1.0f);
  window.MainLoop(
    [&](float delta)
    {
      ctx->ClearResources();
      ctx->TransferPass();
      ctx->RenderPass(framebuffer);

      // change textures at realtime
      if (window.IsKeyPressed(RHI::test_examples::Keycode::KEY_ENTER))
      {
        image_it = std::next(image_it);
        if (image_it == textures.end())
          image_it = textures.begin();
        auto it = image_it;
        for (auto && sampler : samplers)
        {
          sampler->AssignImage(*it);
          it = std::next(it);
          if (it == textures.end())
            it = textures.begin();
        }
      }
    });

  return 0;
}
