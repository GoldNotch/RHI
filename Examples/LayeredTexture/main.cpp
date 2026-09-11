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
  RHI::test_examples::Window window("LayeredTexture", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::ITexture * textureArray =
    UploadLayeredTexture(ctx.get(),
                         {"Grunge_201M.jpg", "Soil_120M.jpg", "Soil_124M.jpg", "Wood_145M.jpg",
                          "Wood_285M.jpg"},
                         false /*with_alpha*/, true /*useMips*/);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  auto * colorAttachment =
    ctx->CreateSurfacedAttachment(window.GetDrawSurface(), RHI::RenderBuffering::Triple);
  framebuffer->AddAttachment(0, colorAttachment);

  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto trianglePipeline = ctx->CreatePipeline();
  trianglePipeline->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  trianglePipeline->AttachShader(RHI::ShaderType::Vertex,
                                 ReadSpirV(FromGLSL("layered_texture.vert")));
  trianglePipeline->AttachShader(RHI::ShaderType::Fragment,
                                 ReadSpirV(FromGLSL("layered_texture.frag")));
  trianglePipeline->DefinePushConstant(sizeof(PushConstant),
                                       RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);
  RHI::ISamplerUniformDescriptor * sampler =
    trianglePipeline->DeclareSampler({0, 0}, RHI::ShaderType::Fragment);
  sampler->AssignImage(textureArray);

  auto process = ctx->CreateProcess();
  {
    auto [width, height] = window.GetSize();
    process->SetViewport(static_cast<float>(width), static_cast<float>(height));
    process->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    constexpr int cells_count_in_row = 8;
    constexpr float cell_width = 2.0f / cells_count_in_row;
    constexpr float offset = -1.0f + cell_width / 2.0f;
    constexpr float margin = 0.05f;
    PushConstant ct;
    ct.scale_x = cell_width / 2.0f;
    ct.scale_y = cell_width / 2.0f;
    ct.texture_index = 0;
    for (int i = 0; i <= cells_count_in_row; ++i)
    {
      for (int j = 0; j <= cells_count_in_row; ++j)
      {
        ct.pos_x = offset + j * (cell_width + margin);
        ct.pos_y = offset + i * (cell_width + margin);
        ct.texture_index = (ct.texture_index + 1) % 5;
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

      if (window.IsKeyPressed(RHI::test_examples::Keycode::KEY_ENTER))
      {
      }
    });

  return 0;
}
