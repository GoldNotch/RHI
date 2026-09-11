#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <future>
#include <span>

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

// helper which incapsulated rendering code for some scene
struct Renderer
{
  explicit Renderer(RHI::IContext & ctx, RHI::IFramebuffer & framebuffer);
  ~Renderer();
  // draw scene in parallel
  void UpdateGeometry(std::span<float> newVertices);

private:
  RHI::IContext * m_context = nullptr;
  RHI::IFramebuffer * m_fbo = nullptr;
  /// subpass which can be executed in parallel
  RHI::PipelinePtr m_pipeline = nullptr;

  /// some data for frame
  RHI::IBufferGPU * m_vertexBuffer = nullptr;
  RHI::IBufferGPU * m_indexBuffer = nullptr;
};

std::atomic_bool m_running = true;
void thread_main(Renderer * renderer)
{
  float t = 0.0;
  while (m_running)
  {
    auto c = std::cosf(t);
    // clang-format off
    std::array<float, 15> newVertices{  
      // pos + colors(rgb)
      0.5f,  0.5f,  c*c, 0.0f, 0.0f, /*first vertex*/
      -0.5f, c*c,  0.0f, 1.0f, 0.0f, /*second vertex*/
      c, -0.5f, 0.0f, 0.0f, 1.0f  /*third vertex*/
    };
    // clang-format on

    renderer->UpdateGeometry(newVertices);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    t += 0.01;
  }
}

int main()
{
  RHI::test_examples::GlfwInstance instance;
  RHI::test_examples::Window window("ParallelPass", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  RHI::IAttachment * colorAttachment =
    ctx->CreateSurfacedAttachment(window.GetDrawSurface(), RHI::RenderBuffering::Triple);
  framebuffer->AddAttachment(0, colorAttachment);

  Renderer triangleRenderer(*ctx, *framebuffer);

  window.onResize = [&triangleRenderer, &framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };
  std::thread newThread(thread_main, &triangleRenderer);

  colorAttachment->SetClearValue(0.1f, 1.0f, 0.4f, 1.0f);
  window.MainLoop(
    [framebuffer, &ctx, &triangleRenderer](float delta)
    {
      ctx->ClearResources();
      ctx->TransferPass();
      ctx->RenderPass(framebuffer);
    });

  m_running = false;
  newThread.join();

  return 0;
}

Renderer::Renderer(RHI::IContext & ctx, RHI::IFramebuffer & framebuffer)
  : m_context(&ctx)
  , m_fbo(&framebuffer)
{
  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  m_pipeline = ctx.CreatePipeline();
  m_pipeline->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  // set shaders
  m_pipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("triangle.vert")));
  m_pipeline->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("triangle.frag")));
  // set vertex attributes (5 float attributes per vertex - pos.xy and color.rgb)
  m_pipeline->AddInputBinding(0, 5 * sizeof(float), RHI::InputBindingType::VertexData);
  m_pipeline->AddInputAttribute(0, 0, 0, 2, RHI::InputAttributeElementType::FLOAT);
  m_pipeline->AddInputAttribute(0, 1, 2 * sizeof(float), 3, RHI::InputAttributeElementType::FLOAT);

  // create vertex buffer
  m_vertexBuffer =
    ctx.CreateBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer, false);
  //m_vertexBuffer->UploadAsync(Vertices, VerticesCount * 5 * sizeof(float));


  // create index buffer
  m_indexBuffer =
    ctx.CreateBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer, false);
  m_indexBuffer->UploadAsync(Indices, IndicesCount * sizeof(uint32_t));

  auto process = ctx.CreateProcess();
  {
    auto extent = m_fbo->GetExtent();
    process->SetViewport(static_cast<float>(extent[0]), static_cast<float>(extent[1]));
    process->SetScissor(0, 0, static_cast<uint32_t>(extent[0]), static_cast<uint32_t>(extent[1]));
    process->BindVertexBuffer(0, m_vertexBuffer, 0);
    process->BindIndexBuffer(m_indexBuffer, RHI::IndexType::UINT32);
    process->DrawIndexedVertices(IndicesCount, 1);
  }
  m_fbo->SetSubpass(0, m_pipeline, process);
}

Renderer::~Renderer()
{
  m_context->DeleteBuffer(m_vertexBuffer);
  m_context->DeleteBuffer(m_indexBuffer);
}

void Renderer::UpdateGeometry(std::span<float> newVertices)
{
  m_vertexBuffer->UploadAsync(newVertices.data(), VerticesCount * 5 * sizeof(float));
}
