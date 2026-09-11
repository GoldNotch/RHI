#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>


static constexpr uint32_t VerticesCount = 3;
static constexpr float Vertices[] = {
  // pos + colors(rgb)
  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, /*first vertex*/
  -0.5f, 0.5f,  0.0f, 1.0f, 0.0f, /*second vertex*/
  -0.0f, -0.5f, 0.0f, 0.0f, 1.0f  /*third vertex*/
};

static constexpr uint32_t IndicesCount = 3;
static constexpr uint32_t Indices[] = {0, 1, 2};

int main()
{
  RHI::test_examples::GlfwInstance instance;

  RHI::test_examples::Window window("Hello triangle", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  auto * surfaceAttachment =
    ctx->CreateSurfacedAttachment(window.GetDrawSurface(), RHI::RenderBuffering::Triple);
  auto * colorAttachment = ctx->CreateAttachment(surfaceAttachment->GetDescription().format,
                                                 surfaceAttachment->GetDescription().extent,
                                                 RHI::RenderBuffering::Triple,
                                                 RHI::SamplesCount::Eight);
  framebuffer->AddAttachment(0, colorAttachment);
  framebuffer->AddAttachment(1, surfaceAttachment);

  window.onResize = [&framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };

  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto trianglePipeline = ctx->CreatePipeline();
  {
    trianglePipeline->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
    trianglePipeline->BindResolver(1, 0);
    // set shaders
    trianglePipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("triangle.vert")));
    trianglePipeline->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("triangle.frag")));
    // set vertex attributes (5 float attributes per vertex - pos.xy and color.rgb)
    trianglePipeline->AddInputBinding(0, 5 * sizeof(float), RHI::InputBindingType::VertexData);
    trianglePipeline->AddInputAttribute(0, 0, 0, 2, RHI::InputAttributeElementType::FLOAT);
    trianglePipeline->AddInputAttribute(0, 1, 2 * sizeof(float), 3,
                                        RHI::InputAttributeElementType::FLOAT);
  }
  // create vertex buffer
  auto * vertexBuffer =
    ctx->CreateBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer, true);
  vertexBuffer->UploadSync(Vertices, VerticesCount * 5 * sizeof(float));

  // create index buffer
  auto * indexBuffer =
    ctx->CreateBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer, true);
  // fill buffer with Mapping into CPU memory
  if (auto scoped_map = indexBuffer->Map())
  {
    std::memcpy(scoped_map.get(), Indices, IndicesCount * sizeof(uint32_t));
    // in the end of scope mapping will be destroyed
  }
  // to make sure that buffer is sent on GPU
  indexBuffer->Flush();

  RHI::PipelineProcessPtr renderProcess = ctx->CreateProcess();
  {
    // get size of window
    auto [width, height] = window.GetSize();
    // set viewport
    renderProcess->SetViewport(static_cast<float>(width), static_cast<float>(height));
    // set scissor
    renderProcess->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    // draw triangle
    renderProcess->BindVertexBuffer(0, vertexBuffer, 0);
    renderProcess->BindIndexBuffer(indexBuffer, RHI::IndexType::UINT32);
    renderProcess->DrawIndexedVertices(IndicesCount, 1);
  }

  framebuffer->SetSubpass(0, trianglePipeline, renderProcess);

  float t = 0.0;
  window.MainLoop(
    [=, &t, &ctx, &surfaceAttachment](float delta)
    {
      colorAttachment->SetClearValue(0.1f, std::abs(std::sin(t)), 0.4f, 1.0f);
      surfaceAttachment->SetClearValue(0, 0, 0, 1);
      ctx->RenderPass(framebuffer);
      t += 0.001f;
    });

  return 0;
}
