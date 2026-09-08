#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Camera.hpp>
#include <glm/ext.hpp>
#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>


void ProcessInput(RHI::test_examples::Window & window, RHI::test_examples::Camera & camera,
                  std::atomic_bool & isRunningFlag)
{
  using namespace RHI::test_examples;

  if (window.IsKeyPressed(Keycode::KEY_ESCAPE))
  {
    isRunningFlag = false;
    window.Close();
  }

  if (window.IsKeyPressed(Keycode::KEY_E))
    window.SetCursorHidden(false);
  else
    window.SetCursorHidden();

  if (window.IsKeyPressed(Keycode::KEY_W))
    camera.MoveCamera(camera.GetFrontVector());
  else if (window.IsKeyPressed(Keycode::KEY_S))
    camera.MoveCamera(-camera.GetFrontVector());

  if (window.IsKeyPressed(Keycode::KEY_A))
    camera.MoveCamera(-camera.GetRightVector());
  else if (window.IsKeyPressed(Keycode::KEY_D))
    camera.MoveCamera(camera.GetRightVector());
}

void SetupCubeInputAttributes(RHI::PipelinePtr pipeline)
{
  // set matrix binding
  pipeline->AddInputBinding(0, sizeof(glm::mat4), RHI::InputBindingType::InstanceData);
  pipeline->AddInputAttribute(0, 0, 0, 4, RHI::InputAttributeElementType::FLOAT);
  pipeline->AddInputAttribute(0, 1, sizeof(glm::vec4), 4, RHI::InputAttributeElementType::FLOAT);
  pipeline->AddInputAttribute(0, 2, 2 * sizeof(glm::vec4), 4,
                              RHI::InputAttributeElementType::FLOAT);
  pipeline->AddInputAttribute(0, 3, 3 * sizeof(glm::vec4), 4,
                              RHI::InputAttributeElementType::FLOAT);
  // set color binding
  pipeline->AddInputBinding(1, sizeof(glm::vec4), RHI::InputBindingType::InstanceData);
  pipeline->AddInputAttribute(1, 4, 0, 4, RHI::InputAttributeElementType::FLOAT);
}

int main()
{
  std::atomic_bool isRunningFlag = true;
  RHI::test_examples::GlfwInstance instance;
  RHI::test_examples::Window window("OIT", 800, 600);
  RHI::test_examples::Camera camera(glm::vec3(0.0f, -1.0f, 0.0f));
  camera.SetAspectRatio(window.GetAspectRatio());

  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  { // create attachments
    auto * surfaceAttachment =
      ctx->CreateSurfacedAttachment(window.GetDrawSurface(), RHI::RenderBuffering::Triple);
    auto * lastColorAttachment = ctx->CreateAttachment(surfaceAttachment->GetDescription().format,
                                                       surfaceAttachment->GetDescription().extent,
                                                       RHI::RenderBuffering::Triple,
                                                       RHI::SamplesCount::Eight);
    auto * depthAttachment = ctx->CreateAttachment(RHI::ImageFormat::DEPTH_STENCIL,
                                                   surfaceAttachment->GetDescription().extent,
                                                   RHI::RenderBuffering::Triple,
                                                   RHI::SamplesCount::Eight);

    auto * sumColorAttachment = ctx->CreateAttachment(surfaceAttachment->GetDescription().format,
                                                      surfaceAttachment->GetDescription().extent,
                                                      RHI::RenderBuffering::Triple,
                                                      RHI::SamplesCount::Eight);
    auto * countFragmentsAttachment =
      ctx->CreateAttachment(RHI::ImageFormat::R8, surfaceAttachment->GetDescription().extent,
                            RHI::RenderBuffering::Triple, RHI::SamplesCount::Eight);

    lastColorAttachment->SetClearValue(0, 0, 0, 0);
    sumColorAttachment->SetClearValue(0, 0, 0, 0);
    countFragmentsAttachment->SetClearValue(0, 0, 0, 0);
    depthAttachment->SetClearValue(1.0, 0);
    surfaceAttachment->SetClearValue(0, 0, 0, 1);

    framebuffer->AddAttachment(0, lastColorAttachment);
    framebuffer->AddAttachment(1, depthAttachment);
    framebuffer->AddAttachment(2, sumColorAttachment);
    framebuffer->AddAttachment(3, countFragmentsAttachment);
    framebuffer->AddAttachment(4, surfaceAttachment);
  }

  window.onResize = [&framebuffer](int width, int height)
  {
    framebuffer->Resize(width, height);
  };

  window.onMoveCursor = [&camera](double xpos, double ypos, double dx, double dy)
  {
    camera.OnCursorMoved({dx, dy});
  };

  const uint32_t c_MaxCubesCount = 5;
  std::array<glm::mat4, c_MaxCubesCount> cubesTransforms{
    glm::mat4(100, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 100.0, 0, 0, 0, 0, 1),
    glm::mat4(2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0.1, 0, 1),
    glm::mat4(1, 0, 0, 0, 0, 3, 0, 0, 0, 0, 1, 0, 5, 0.1, 10, 1),
    glm::mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 4, 0.1, 7, 1),
    glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 8, 0.1, 3, 1),
  };
  std::array<glm::vec4, c_MaxCubesCount> cubesColors{
    glm::vec4(0.4, 0.4, 0.4, 1), glm::vec4(0.2, 1, 0.3, 1), glm::vec4(0.2, 0.4, 0.7, 1),
    glm::vec4(1, 0.4, 0.7, 1),   glm::vec4(0.5, 0.6, 0, 1),
  };

  // buffers
  auto viewProjUniformBuffer =
    ctx->CreateBuffer(sizeof(glm::mat4), RHI::BufferGPUUsage::UniformBuffer, true);
  auto cubesTransformBuffer = ctx->CreateBuffer(c_MaxCubesCount * sizeof(glm::mat4),
                                                RHI::BufferGPUUsage::VertexBuffer, false);
  auto cubesColorsBuffer = ctx->CreateBuffer(c_MaxCubesCount * sizeof(glm::vec4),
                                             RHI::BufferGPUUsage::VertexBuffer, false);
  cubesTransformBuffer->UploadAsync(cubesTransforms.data(),
                                    cubesTransforms.size() * sizeof(glm::mat4));
  cubesColorsBuffer->UploadAsync(cubesColors.data(), cubesColors.size() * sizeof(glm::vec4));

  // pipelines

  auto opacityPipeline = ctx->CreatePipeline();
  {
    // setup attachments
    opacityPipeline->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
    opacityPipeline->BindAttachment(1, RHI::ShaderAttachmentSlot::DepthStencil);
    opacityPipeline->BindAttachment(2, RHI::ShaderAttachmentSlot::Preserved);
    opacityPipeline->BindAttachment(3, RHI::ShaderAttachmentSlot::Preserved);
    opacityPipeline->BindAttachment(4, RHI::ShaderAttachmentSlot::Preserved);
    //opacityPipeline->BindResolver(4, 0);
    // set shaders
    opacityPipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("cube.vert")));
    opacityPipeline->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("cube.frag")));
    opacityPipeline->SetMeshTopology(RHI::MeshTopology::Triangle);
    opacityPipeline->EnableDepthTest(true);
    // uniforms
    auto viewProjUniform = opacityPipeline->DeclareUniform({0, 0}, RHI::ShaderType::Vertex);
    viewProjUniform->AssignBuffer(viewProjUniformBuffer, 0);
    // set attributes
    SetupCubeInputAttributes(opacityPipeline);
  }

  //auto accumPipeline = framebuffer->CreatePipeline();
  //{
  //  // setup attachments
  //  accumPipeline->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
  //  accumPipeline->BindAttachment(1, RHI::ShaderAttachmentSlot::DepthStencil);
  //  accumPipeline->BindAttachment(2, RHI::ShaderAttachmentSlot::Color);
  //  accumPipeline->BindAttachment(3, RHI::ShaderAttachmentSlot::Color);
  //  accumPipeline->BindAttachment(4, RHI::ShaderAttachmentSlot::Preserved);
  //  // set shaders
  //  accumPipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("cube.vert")));
  //  accumPipeline->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("oit_accum.frag")));
  //  accumPipeline->SetMeshTopology(RHI::MeshTopology::Triangle);
  //  accumPipeline->EnableDepthTest(false);
  //  // uniforms
  //  auto viewProjUniform = opacityPipeline->DeclareUniform({0, 1}, RHI::ShaderType::Vertex);
  //  viewProjUniform->AssignBuffer(viewProjUniformBuffer, 0);
  //  // set attributes
  //  SetupCubeInputAttributes(opacityPipeline);
  //}

  auto summaryPipeline = ctx->CreatePipeline();
  {
    // setup attachments
    summaryPipeline->BindAttachment(0,
                                    RHI::ShaderAttachmentSlot::Color |
                                      RHI::ShaderAttachmentSlot::Input,
                                    {0, 0});
    summaryPipeline->BindAttachment(1, RHI::ShaderAttachmentSlot::Preserved);
    summaryPipeline->BindAttachment(2, RHI::ShaderAttachmentSlot::Preserved);
    summaryPipeline->BindAttachment(3, RHI::ShaderAttachmentSlot::Preserved);
    summaryPipeline->BindResolver(4, 0);
    // set shaders
    summaryPipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("quad.vert")));
    summaryPipeline->AttachShader(RHI::ShaderType::Fragment,
                                  ReadSpirV(FromGLSL("oit_summary.frag")));
    summaryPipeline->SetMeshTopology(RHI::MeshTopology::TriangleFan);
  }

  // processes

  RHI::PipelineProcessPtr opacityProcess = ctx->CreateProcess();
  {
    auto [width, height] = window.GetSize();
    opacityProcess->SetViewport(width, height);
    opacityProcess->SetScissor(0, 0, width, height);
    opacityProcess->BindVertexBuffer(0, cubesTransformBuffer, 0);
    opacityProcess->BindVertexBuffer(1, cubesColorsBuffer, 0);
    opacityProcess->DrawVertices(36, c_MaxCubesCount);
  }


  RHI::PipelineProcessPtr accumProcess = ctx->CreateProcess();
  {
    auto [width, height] = window.GetSize();
    accumProcess->SetViewport(width, height);
    accumProcess->SetScissor(0, 0, width, height);
    accumProcess->DrawVertices(36, c_MaxCubesCount);
  }
  //accumPipeline->SetRenderProcess(std::move(accumProcess));

  RHI::PipelineProcessPtr summaryProcess = ctx->CreateProcess();
  {
    auto [width, height] = window.GetSize();
    summaryProcess->SetViewport(width, height);
    summaryProcess->SetScissor(0, 0, width, height);
    summaryProcess->DrawVertices(4, 1);
  }
  framebuffer->SetSubpass(0, opacityPipeline, std::move(opacityProcess));
  framebuffer->SetSubpass(1, summaryPipeline, summaryProcess);

  window.MainLoop(
    [=, &ctx, &window, &camera, &isRunningFlag](float delta)
    {
      ProcessInput(window, camera, isRunningFlag);
      glm::mat4 vp = camera.GetVP();
      viewProjUniformBuffer->UploadSync(&vp, sizeof(glm::mat4));

      ctx->ClearResources();
      ctx->RenderPass(framebuffer);
      ctx->TransferPass();
    });

  return 0;
}
