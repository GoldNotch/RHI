#include "Renderer.hpp"

#include <glm/ext.hpp>
#include <TestUtils.hpp>

CubesRenderer::CubesRenderer(RHI::IContext & ctx)
  : m_context(ctx)
  , m_uniformBuffer(
      ctx.CreateBuffer(sizeof(UniformBlock), RHI::BufferGPUUsage::UniformBuffer, true))
  , m_cubesBuffer(ctx.CreateBuffer(sizeof(CubeDescription) * g_MaxCubesCount,
                                   RHI::BufferGPUUsage::VertexBuffer, false))
{
  SetCameraTransform(glm::identity<glm::mat4>());

  auto pipeline = ctx.CreatePipeline();
  {
      pipeline->BindAttachment(0, RHI::ShaderAttachmentSlot::Color);
      pipeline->BindAttachment(1, RHI::ShaderAttachmentSlot::DepthStencil);
      pipeline->BindResolver(2, 0);
      pipeline->EnableDepthTest(true);
      // set shaders
      pipeline->AttachShader(RHI::ShaderType::Vertex, ReadSpirV(FromGLSL("cube.vert")));
      pipeline->AttachShader(RHI::ShaderType::Geometry, ReadSpirV(FromGLSL("cube.geom")));
      pipeline->AttachShader(RHI::ShaderType::Fragment, ReadSpirV(FromGLSL("cube.frag")));
      pipeline->SetMeshTopology(RHI::MeshTopology::Point);

      pipeline->AddInputBinding(0, sizeof(CubeDescription), RHI::InputBindingType::VertexData);
      pipeline->AddInputAttribute(0, 0, 0, 3, RHI::InputAttributeElementType::FLOAT);
      pipeline->AddInputAttribute(0, 1, 3 * sizeof(float), 3, RHI::InputAttributeElementType::FLOAT);
      pipeline->AddInputAttribute(0, 2, 6 * sizeof(float), 3, RHI::InputAttributeElementType::FLOAT);
      pipeline->AddInputAttribute(0, 3, 9 * sizeof(float), 1, RHI::InputAttributeElementType::SINT);


      auto* uniform = pipeline->DeclareUniform({ 0, 0 }, RHI::ShaderType::Vertex);
      uniform->AssignBuffer(m_uniformBuffer);
      pipeline->DeclareSamplersArray({ 0, 1 }, RHI::ShaderType::Fragment,
          static_cast<uint32_t>(m_textures.size()), m_textures.data());
      for (auto* texture : m_textures)
          texture->SetFilter(RHI::TextureFilteration::Linear, RHI::TextureFilteration::Linear);
  }
  m_renderPass = pipeline;
}

CubesRenderer::~CubesRenderer()
{
  m_context.DeleteBuffer(m_uniformBuffer);
  m_context.DeleteBuffer(m_cubesBuffer);
  DestroyHandles();
}

void CubesRenderer::BindDrawSurface(RHI::IFramebuffer * framebuffer)
{
  DestroyHandles();

  m_drawSurface = framebuffer;

}

size_t CubesRenderer::AddCubeToScene(const CubeDescription & description)
{
  size_t result = m_cubesCount;
  m_cubes[m_cubesCount++] = description;
  m_invalidGeometry = true;
  return result;
}

void CubesRenderer::DeleteCubeFromScene(size_t cubeId)
{
  if (cubeId != m_cubesCount - 1)
    std::swap(m_cubes[cubeId], m_cubes[m_cubesCount - 1]);
  m_cubesCount--;
  m_invalidGeometry = true;
}

void CubesRenderer::SetCameraTransform(const glm::mat4 & vp)
{
  m_uniformBuffer->UploadAsync(&vp, sizeof(glm::mat4), offsetof(UniformBlock, vp));
}

void CubesRenderer::BindTexture(uint32_t idx, RHI::ITexture * texture)
{
  if (m_textures[idx])
    m_textures[idx]->AssignImage(texture);
}

void CubesRenderer::Draw()
{
  if (m_invalidGeometry && m_cubesCount > 0)
  {
    m_cubesBuffer->UploadAsync(m_cubes.data(), m_cubesCount * sizeof(CubeDescription));
    m_invalidGeometry = false;
    InvalidateScene();
  }
}

void CubesRenderer::InvalidateScene()
{
  auto process = m_context.CreateProcess();
  {
    auto extent = m_drawSurface->GetExtent();
    uint32_t width = extent[0], height = extent[1];
    process->SetViewport(static_cast<float>(width), static_cast<float>(height));
    process->SetScissor(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    process->BindVertexBuffer(0, m_cubesBuffer);
    process->DrawVertices(static_cast<uint32_t>(m_cubesCount), 1);
  }
  m_drawSurface->SetSubpass(0, m_renderPass, process);
}

void CubesRenderer::DestroyHandles()
{
  //TODO: remove m_renderPass
}

CubesRenderer::UniformBlock::UniformBlock()
  : vp(glm::identity<glm::mat4>())
{
}
