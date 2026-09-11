#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

int main()
{
  RHI::test_examples::GlfwInstance instance;
  RHI::test_examples::Window window1("Multiwindow_1", 800, 600);
  RHI::test_examples::Window window2("Multiwindow_2", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::IFramebuffer * framebuffer1 = ctx->CreateFramebuffer();
  RHI::IFramebuffer * framebuffer2 = ctx->CreateFramebuffer();
  auto * surface1 =
    ctx->CreateSurfacedAttachment(window1.GetDrawSurface(), RHI::RenderBuffering::Triple);
  auto * surface2 =
    ctx->CreateSurfacedAttachment(window2.GetDrawSurface(), RHI::RenderBuffering::Triple);
  framebuffer1->AddAttachment(0, surface1);
  framebuffer2->AddAttachment(0, surface2);

  auto pipeline1 = ctx->CreatePipeline();
  {
    pipeline1->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
    // set shaders
    pipeline1->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("triangle.vert")));
    pipeline1->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("triangle_quad.frag")));
    pipeline1->SetMeshTopology(RHI::MeshTopology::Triangle);
    auto process = ctx->CreateProcess();
    {
      auto [width, height] = window1.GetSize();
      process->SetViewport(static_cast<float>(width), static_cast<float>(height));
      process->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
      process->DrawVertices(3, 1);
    }
    framebuffer1->SetSubpass(0, pipeline1, process);
  }

  auto pipeline2 = ctx->CreatePipeline();
  {
    pipeline2->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
    // set shaders
    pipeline2->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("quad.vert")));
    pipeline2->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("triangle_quad.frag")));
    pipeline2->SetMeshTopology(RHI::MeshTopology::TriangleStrip);
    auto process = ctx->CreateProcess();
    {
      auto [width, height] = window2.GetSize();
      process->SetViewport(static_cast<float>(width), static_cast<float>(height));
      process->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
      process->DrawVertices(4, 1);
    }
    framebuffer2->SetSubpass(0, pipeline2, process);
  }

  float t = 0.0;
  window1.MainLoop(
    [&](float delta)
    {
      surface1->SetClearValue(0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
      surface2->SetClearValue(0.1f, std::abs(std::cos(t)), 0.4f, 1.0f);
      ctx->ClearResources();
      ctx->RenderPass(framebuffer1);
      ctx->RenderPass(framebuffer2);
      t += 0.0001;
    });

  return 0;
}
