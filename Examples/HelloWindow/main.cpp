#include <cassert>
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

  RHI::test_examples::Window window("Hello window", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);
  assert(ctx);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  RHI::IAttachment * colorAttachment =
    ctx->CreateSurfacedAttachment(window.GetDrawSurface(), RHI::RenderBuffering::Double);
  framebuffer->AddAttachment(0, colorAttachment);

  window.onResize = [&framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };

  float t = 0.0;
  window.MainLoop(
    [=, &t, &ctx](float delta)
    {
      colorAttachment->SetClearValue(0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
      ctx->RenderPass(framebuffer);
      t += 0.001f;
    });

  return 0;
}
