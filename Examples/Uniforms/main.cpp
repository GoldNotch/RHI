#include <cmath>
#include <cstdio>
#include <cstring>

#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

static constexpr uint32_t VerticesCount = 4;
static constexpr float Vertices[] = {
  // pos + colors(rgb)
  0.5f,  0.5f,  0.227f, 0.117f, 0.729f, /*first vertex*/
  -0.5f, 0.5f,  0.729f, 0.117f, 0.227f, /*second vertex*/
  -0.5f, -0.5f, 0.619f, 0.729f, 0.117f, /*third vertex*/
  0.5f,  -0.5f, 0.117f, 0.729f, 0.533f  /*fourth vertex*/
};

static constexpr uint32_t IndicesCount = 6;
static constexpr uint32_t Indices[] = {0, 1, 2, 0, 2, 3};

int main()
{
  RHI::test_examples::GlfwInstance instance;

  RHI::test_examples::Window window("Uniforms", 800, 600);

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  // create buffers for each uniform variables
  auto tBuf = ctx->CreateBuffer(sizeof(float), RHI::BufferGPUUsage::UniformBuffer, true);
  auto transformBuf =
    ctx->CreateBuffer(2 * sizeof(float), RHI::BufferGPUUsage::UniformBuffer, true);

  auto * framebuffer = ctx->CreateFramebuffer();
  auto * colorAttachment =
    ctx->CreateSurfacedAttachment(window.GetDrawSurface(), RHI::RenderBuffering::Triple);
  framebuffer->AddAttachment(0, colorAttachment);

  auto trianglePipeline = ctx->CreatePipeline();
  trianglePipeline->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  trianglePipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("uniform.vert")));
  trianglePipeline->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("uniform.frag")));
  // set vertex attributes (5 float attributes per vertex - pos.xy and color.rgb)
  trianglePipeline->AddInputBinding(0, 5 * sizeof(float), RHI::InputBindingType::VertexData);
  trianglePipeline->AddInputAttribute(0, 0, 0, 2, RHI::InputAttributeElementType::FLOAT);
  trianglePipeline->AddInputAttribute(0, 1, 2 * sizeof(float), 3,
                                      RHI::InputAttributeElementType::FLOAT);

  // declare uniform variables
  auto && u_t =
    trianglePipeline->DeclareUniform({0, 0}, RHI::ShaderType::Fragment | RHI::ShaderType::Vertex);
  u_t->AssignBuffer(tBuf); // bind buffer to uniform variable

  auto && u_transform = trianglePipeline->DeclareUniform({0, 1}, RHI::ShaderType::Vertex);
  u_transform->AssignBuffer(transformBuf); // bind buffer to uniform variable

  // create vertex buffer
  auto && vertexBuffer =
    ctx->CreateBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer, false);
  vertexBuffer->UploadAsync(Vertices, VerticesCount * 5 * sizeof(float));

  // create index buffer
  auto indexBuffer =
    ctx->CreateBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer, false);
  indexBuffer->UploadAsync(Indices, IndicesCount * sizeof(uint32_t));

  window.onResize = [framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };

  auto process = ctx->CreateProcess();
  {
    auto [width, height, _] = framebuffer->GetExtent();
    // set viewport
    process->SetViewport(static_cast<float>(width), static_cast<float>(height));
    // set scissor
    process->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    // draw triangle
    process->BindVertexBuffer(0, vertexBuffer, 0);
    process->BindIndexBuffer(indexBuffer, RHI::IndexType::UINT32);
    process->DrawIndexedVertices(IndicesCount, 1);
  }
  framebuffer->SetSubpass(0,trianglePipeline, std::move(process));

  float x = 0.0f;
  colorAttachment->SetClearValue(0.3f, 0.3f, 0.5f, 1.0f);
  window.MainLoop(
    [&](float delta)
    {
      float t_val = std::abs(std::sin(x));
      tBuf->UploadSync(&t_val, sizeof(float));

      std::pair<float, float> transform_val{std::cos(x), std::sin(x)};
      auto task = transformBuf->UploadAsync(&transform_val, 2 * sizeof(float));
      std::array<const RHI::IAwaitable *, 1> prerenderTasks;
      prerenderTasks[0] = task.get();

      x += 0.001f;
      ctx->ClearResources();
      ctx->TransferPass();
      ctx->RenderPass(framebuffer, prerenderTasks);
    });

  return 0;
}
