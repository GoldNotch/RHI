#pragma once
#include <future>

#include <glm/glm.hpp>
#include <RHI.hpp>

class CubesRenderer final
{
  static constexpr size_t g_MaxTexturesCount = 8;

  /// global data for all cubes
  struct UniformBlock
  {
    glm::mat4 vp; ///< camera transform: view + projection
    UniformBlock();
  };

public:
  static constexpr size_t g_MaxCubesCount = 100; ///< max cubes count in scene
  /// describes one cube in scene
  struct CubeDescription
  {
    glm::vec3 pos{};
    glm::vec3 axis{0, 0, 1};
    glm::vec3 scale{1.0f};
    int32_t textureIndex = 0;
  };

  explicit CubesRenderer(RHI::IContext & ctx);
  ~CubesRenderer();

public: // Client API
  void BindDrawSurface(RHI::IFramebuffer * framebuffer);
  size_t AddCubeToScene(const CubeDescription & description);
  void DeleteCubeFromScene(size_t cubeId);
  void SetCameraTransform(const glm::mat4 & vp);
  void BindTexture(uint32_t idx, RHI::ITexture * texture);
  void InvalidateScene();
  size_t GetCubesCount() const noexcept { return m_cubesCount; }

public: // RenderThread API
  void Draw();

private:
  std::array<CubeDescription, g_MaxCubesCount> m_cubes;
  size_t m_cubesCount = 0; ///< count on scene

  RHI::IContext & m_context;
  RHI::IFramebuffer * m_drawSurface = nullptr; ///< surface where to draw

  RHI::IBufferGPU * m_uniformBuffer = nullptr; ///< buffer for uniform
  RHI::IBufferGPU * m_cubesBuffer = nullptr;
  RHI::PipelinePtr m_renderPass = nullptr; ///< renderPass for rendering cubes
  std::array<RHI::ISamplerUniformDescriptor *, g_MaxTexturesCount> m_textures;

  /// current draw task
  std::future<bool> m_drawTask;
  bool m_invalidScene = true;
  bool m_invalidGeometry = false;

private:
  void DestroyHandles();
};
