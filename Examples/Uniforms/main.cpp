#include <cstdio>

#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>
#include <RHI.hpp>

// Custom log function used by RHI::Context
void ConsoleLog(RHI::LogMessageStatus status, const std::string & message)
{
  switch (status)
  {
    case RHI::LogMessageStatus::LOG_INFO:
      std::printf("INFO: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_WARNING:
      std::printf("WARNING: - %s\n", message.c_str());
      break;
    case RHI::LogMessageStatus::LOG_ERROR:
      std::printf("ERROR: - %s\n", message.c_str());
      break;
  }
}

// flag means that you should clear and update trianglePipelineCommands (see in main)
bool ShouldInvalidateScene = true;

// Resize window callback
void OnResizeWindow(GLFWwindow * window, int width, int height)
{
  RHI::IContext * ctx = reinterpret_cast<RHI::IContext *>(glfwGetWindowUserPointer(window));
  ctx->GetSwapchain().Invalidate();
  ShouldInvalidateScene = true;
}

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
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create GLFW window
  GLFWwindow * window = glfwCreateWindow(800, 600, "Uniforms_RHI", NULL, NULL);
  if (window == NULL)
  {
    std::printf("Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }
  // set callback on resize
  glfwSetWindowSizeCallback(window, OnResizeWindow);

  // fill structure for surface with OS handles
  RHI::SurfaceConfig surface{};
#ifdef _WIN32
  surface.hWnd = glfwGetWin32Window(window);
  surface.hInstance = GetModuleHandle(nullptr);
#elif defined(__linux__)
  surface.hWnd = reinterpret_cast<void *>(glfwGetX11Window(window));
  surface.hInstance = glfwGetX11Display();
#endif

  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(surface, ConsoleLog);
  glfwSetWindowUserPointer(window, ctx.get());

  // pipeline must be associated with some framebuffer.
  // We want to draw info window surface so we must attach pipeline to DefaultFramebuffer (like OpenGL)
  auto && defaultFramebuffer = ctx->GetSwapchain().GetDefaultFramebuffer();

  // create pipeline for triangle. Here we can configure gpu pipeline for rendering
  auto && trianglePipeline = ctx->CreatePipeline(defaultFramebuffer, 0 /*index of subpass*/);
  // set shaders
  trianglePipeline->AttachShader(RHI::ShaderType::Vertex, L"Shaders/uniform_vert.spv");
  trianglePipeline->AttachShader(RHI::ShaderType::Fragment, L"Shaders/uniform_frag.spv");
  // set vertex attributes (5 float attributes per vertex - pos.xy and color.rgb)
  trianglePipeline->AddInputBinding(0, 5 * sizeof(float), RHI::InputBindingType::VertexData);
  trianglePipeline->AddInputAttribute(0, 0, 0, 2, RHI::InputAttributeElementType::FLOAT);
  trianglePipeline->AddInputAttribute(0, 1, 2 * sizeof(float), 3,
                                      RHI::InputAttributeElementType::FLOAT);

  auto && tbuf =
    trianglePipeline->DeclareUniform("ub", 0, RHI::ShaderType::Fragment | RHI::ShaderType::Vertex,
                                     sizeof(float));

  auto && transformBuf =
    trianglePipeline->DeclareUniform("tb", 1, RHI::ShaderType::Vertex, 2 * sizeof(float));
  // don't forget to call Invalidate to apply all changed settings
  trianglePipeline->Invalidate();

  // create vertex buffer
  auto && vertexBuffer =
    ctx->AllocBuffer(VerticesCount * 5 * sizeof(float), RHI::BufferGPUUsage::VertexBuffer);
  // fill buffer with Mapping into CPU memory
  if (auto scoped_map = vertexBuffer->Map())
  {
    std::memcpy(scoped_map.get(), Vertices, VerticesCount * 5 * sizeof(float));
    // in the end of scope mapping will be destroyed
  }
  // to make sure that buffer is sent on GPU
  vertexBuffer->Flush();


  // create index buffer
  auto indexBuffer =
    ctx->AllocBuffer(IndicesCount * sizeof(uint32_t), RHI::BufferGPUUsage::IndexBuffer);
  // fill buffer with Mapping into CPU memory
  if (auto scoped_map = indexBuffer->Map())
  {
    std::memcpy(scoped_map.get(), Indices, IndicesCount * sizeof(uint32_t));
    // in the end of scope mapping will be destroyed
  }
  // to make sure that buffer is sent on GPU
  indexBuffer->Flush();


  // command buffer for drawing triangle
  auto && trianglePipelineCommands = ctx->GetSwapchain().CreateCommandBuffer();
  ShouldInvalidateScene = true;
  float x = 0.0f;
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    // fill trianglePipelineCommands
    if (ShouldInvalidateScene)
    {
      // get size of window
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      // clear commands buffer
      trianglePipelineCommands->Reset();
      // enter in editing mode.
      trianglePipelineCommands->BeginWriting(defaultFramebuffer, *trianglePipeline);
      // here we can push all commands you want

      // set viewport
      trianglePipelineCommands->SetViewport(static_cast<float>(width), static_cast<float>(height));
      // set scissor
      trianglePipelineCommands->SetScissor(0, 0, static_cast<uint32_t>(width),
                                           static_cast<uint32_t>(height));
      // draw triangle
      trianglePipelineCommands->BindVertexBuffer(0, *vertexBuffer, 0);
      trianglePipelineCommands->BindIndexBuffer(*indexBuffer, RHI::IndexType::UINT32);
      trianglePipelineCommands->DrawIndexedVertices(IndicesCount, 1);

      // finish editing mode
      trianglePipelineCommands->EndWriting();

      ShouldInvalidateScene = false;
    }

    if (auto map = tbuf->Map())
    {
      float t_val = std::abs(std::sinf(x));
      std::memcpy(map.get(), &t_val, sizeof(float));
    }

    if (auto map = transformBuf->Map())
    {
      float cos_val = std::cosf(x);
      float sin_val = std::sinf(x);
      std::memcpy(map.get(), &sin_val, sizeof(float));
      std::memcpy(reinterpret_cast<char *>(map.get()) + sizeof(float), &cos_val, sizeof(float));
    }

    x += 0.0001;

    // swapchain used as generic interface for drawing on window
    auto && swapchain = ctx->GetSwapchain();
    // begin frame returns Command buffer you should fill.
    // It's empty on this step.
    // Written commands will be upload to GPU and executed
    auto && commands = swapchain.BeginFrame({0.3f, 0.3f, 0.5f, 1.0f});
    // push trianglePipelineCommands to CommandBuffer for drawing
    commands->AddCommands(*trianglePipelineCommands);
    // finish frame and output image on window
    swapchain.EndFrame();
  }

  // wait while gpu is idle to destroy context and its objects correctly
  ctx->WaitForIdle();
  glfwTerminate();
  return 0;
}
