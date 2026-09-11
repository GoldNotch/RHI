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
};

int main()
{
  RHI::test_examples::GlfwInstance instance;
  RHI::test_examples::Window window("TextureAtlas", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::ITexture * texture = BuildAtlasTexture(ctx.get(), 2048,
                                              {"texture.png", "jackal.jpg", "shrek1.jpg",
                                               "shrek2.jpg", "mike_wazowski.jpg", "pepe.jpg"},
                                              true, true);


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
  trianglePipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("textureAtlas.vert")));
  trianglePipeline->AttachShader(RHI::ShaderType::Fragment,
                                 ReadSpirV(FromGLSL("textureAtlas.frag")));
  trianglePipeline->DefinePushConstant(sizeof(PushConstant),
                                       RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);

  {
    auto * sampler = trianglePipeline->DeclareSampler({0, 0}, RHI::ShaderType::Fragment);
    sampler->SetFilter(RHI::TextureFilteration::Linear, RHI::TextureFilteration::Linear);
    sampler->AssignImage(texture);
  }
  auto process = ctx->CreateProcess();
  {
    // get size of window
    auto [width, height] = window.GetSize();
    process->SetViewport(static_cast<float>(width), static_cast<float>(height));
    process->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    process->DrawVertices(6, 1);
  }
  framebuffer->SetSubpass(0, trianglePipeline, process);

  colorAttachment->SetClearValue(0.3f, 0.3f, 0.5f, 1.0f);
  window.MainLoop(
    [&](float delta)
    {
      ctx->ClearResources();
      ctx->TransferPass();
      ctx->RenderPass(framebuffer);
    });

  return 0;
}
