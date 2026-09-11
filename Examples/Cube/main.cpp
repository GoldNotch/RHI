#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include <Camera.hpp>
#include <glm/ext.hpp>
#include <RHI.hpp>
#include <TestUtils.hpp>
#include <Window.hpp>

#include "Renderer.hpp"

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

void game_thread_main(CubesRenderer & renderer, std::atomic_bool & isRunningFlag)
{
  constexpr double st_sceneActionChance = 0.3;
  isRunningFlag = true;

  {
    CubesRenderer::CubeDescription cube;
    cube.pos = {0, 0, 0};
    cube.scale = {10.0, 0.1, 10.0};
    cube.textureIndex = 0;
    renderer.AddCubeToScene(cube);
  }

  for (int i = 0; i < 25; ++i)
  {
    CubesRenderer::CubeDescription cube;
    cube.pos = glm::ballRand(10.0);
    cube.pos.y = glm::abs(cube.pos.y);
    cube.scale = glm::vec3(glm::gaussRand(0.5f, 0.49f));
    cube.textureIndex = 1;
    renderer.AddCubeToScene(cube);
  }


  while (isRunningFlag)
  {
    double chanceValue = glm::linearRand(0.0, 1.0);
    if (chanceValue < st_sceneActionChance)
    {
      bool isRemove = renderer.GetCubesCount() > 1 &&
                      (renderer.GetCubesCount() >= CubesRenderer::g_MaxCubesCount ||
                       static_cast<bool>(std::rand() % 2));
      if (isRemove)
      {
        // we shouldn't remove first cube because it's floor
        size_t randCubeIdx = std::rand() % (renderer.GetCubesCount() - 1);
        renderer.DeleteCubeFromScene(randCubeIdx + 1);
      }
      else if (renderer.GetCubesCount() < CubesRenderer::g_MaxCubesCount)
      {
        CubesRenderer::CubeDescription cube;
        cube.pos = glm::ballRand(10.0);
        cube.axis = glm::ballRand(1.0);
        cube.pos.y = glm::abs(cube.pos.y);
        cube.scale = glm::vec3(glm::gaussRand(0.5f, 0.49f));
        cube.textureIndex = 1;
        renderer.AddCubeToScene(cube);
      }
    }

    renderer.Draw();
    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(10));
  }
}

int main()
{
  RHI::test_examples::GlfwInstance instance;

  RHI::test_examples::Window window("Cubes", 800, 600);
  window.SetCursorHidden();
  static constexpr glm::vec3 cameraUp(0.0f, -1.0f, 0.0f);
  RHI::test_examples::Camera camera(cameraUp);
  camera.SetAspectRatio(window.GetAspectRatio());


  RHI::GpuTraits gpuTraits{};
  gpuTraits.require_presentation = true;
  gpuTraits.require_geometry_shaders = true;
  std::unique_ptr<RHI::IContext> ctx = RHI::CreateContext(gpuTraits, ConsoleLog);

  RHI::IFramebuffer * framebuffer = ctx->CreateFramebuffer();
  auto * surfaceAttachment =
    ctx->CreateSurfacedAttachment(window.GetDrawSurface(), RHI::RenderBuffering::Triple);
  auto * depthAttachment = ctx->CreateAttachment(RHI::ImageFormat::DEPTH_STENCIL,
                                                 surfaceAttachment->GetDescription().extent,
                                                 RHI::RenderBuffering::Triple,
                                                 RHI::SamplesCount::Eight);
  auto * colorAttachment =
    ctx->CreateAttachment(RHI::ImageFormat::RGBA8, surfaceAttachment->GetDescription().extent,
                          RHI::RenderBuffering::Triple, RHI::SamplesCount::Eight);

  framebuffer->AddAttachment(2, surfaceAttachment);
  framebuffer->AddAttachment(1, depthAttachment);
  framebuffer->AddAttachment(0, colorAttachment);

  CubesRenderer renderer(*ctx);
  renderer.BindDrawSurface(framebuffer);
  renderer.BindTexture(0, UploadTexture("floor.jpg", ctx.get(), false));
  renderer.BindTexture(1, UploadTexture("container.png", ctx.get(), false));

  window.onResize = [framebuffer, &window, &camera, &renderer](int width, int height)
  {
    framebuffer->Resize(width, height);
    camera.SetAspectRatio(window.GetAspectRatio());
    renderer.InvalidateScene();
  };

  window.onMoveCursor = [&camera](double xpos, double ypos, double dx, double dy)
  {
    camera.OnCursorMoved({dx, dy});
  };

  std::atomic_bool isRunningFlag = false;
  std::thread game_thread(game_thread_main, std::ref(renderer), std::ref(isRunningFlag));

  surfaceAttachment->SetClearValue(0, 0, 0, 1);
  colorAttachment->SetClearValue(0.1f, 0.2f, 0.4f, 1.0f);
  depthAttachment->SetClearValue(1.0f, 0);
  window.MainLoop(
    [&](float delta)
    {
      ProcessInput(window, camera, isRunningFlag);
      renderer.SetCameraTransform(camera.GetVP());
      const auto start{std::chrono::steady_clock::now()};
      ctx->ClearResources();
      ctx->TransferPass();
      ctx->RenderPass(framebuffer);
      const auto finish{std::chrono::steady_clock::now()};
      const std::chrono::duration<double, std::milli> elapsed_ms{finish - start};
      std::printf("FPS: %.1f(%.3f)\n", 1000.0 / elapsed_ms.count(), elapsed_ms.count());
    });

  game_thread.join();
  return 0;
}
