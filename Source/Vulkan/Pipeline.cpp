#include "Pipeline.hpp"

#include "DescriptorsBuffer.hpp"
#include "Framebuffer.hpp"
#include "Utils/Builders.hpp"
namespace RHI::vulkan
{

Pipeline::Pipeline(const Context & ctx, const IFramebuffer & framebuffer, uint32_t subpassIndex)
  : m_owner(ctx)
  , m_framebuffer(framebuffer)
  , m_subpassIndex(subpassIndex)
{
  m_descriptors = std::make_unique<DescriptorBuffer>(ctx);
  m_layoutBuilder = std::make_unique<details::PipelineLayoutBuilder>();
  m_pipelineBuilder = std::make_unique<details::PipelineBuilder>();
}

Pipeline::~Pipeline()
{
  if (!!m_pipeline)
    vkDestroyPipeline(m_owner.GetDevice(), m_pipeline, nullptr);
  if (!!m_layout)
    vkDestroyPipelineLayout(m_owner.GetDevice(), m_layout, nullptr);
}

void Pipeline::AttachShader(ShaderType type, const wchar_t * path)
{
  m_pipelineBuilder->AttachShader(type, path);
  m_invalidPipeline = true;
}

void Pipeline::AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type)
{
  m_pipelineBuilder->AddInputBinding(slot, stride, type);
  m_invalidPipeline = true;
}

void Pipeline::AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType)
{
  m_pipelineBuilder->AddInputAttribute(binding, location, offset, elemsCount, elemsType);
  m_invalidPipeline = true;
}

IBufferGPU * Pipeline::DeclareUniform(const char * name, uint32_t binding, ShaderType shaderStage,
                                      uint32_t size)
{
  return m_descriptors->DeclareUniform(binding, shaderStage, size);
}

void Pipeline::Invalidate()
{
  m_descriptors->Invalidate();

  if (m_invalidPipelineLayout || !m_layout)
  {
    auto new_layout = m_layoutBuilder->Make(m_owner.GetDevice(), m_descriptors->GetLayoutHandle());
    if (!!m_layout)
      vkDestroyPipelineLayout(m_owner.GetDevice(), m_layout, nullptr);
    m_layout = new_layout;
    m_invalidPipelineLayout = false;
    m_invalidPipeline = true;
  }

  if (m_invalidPipeline || !m_pipeline)
  {
    auto new_pipeline =
      m_pipelineBuilder->Make(m_owner.GetDevice(),
                              reinterpret_cast<VkRenderPass>(m_framebuffer.GetRenderPass()),
                              m_subpassIndex, m_layout);
    if (!!m_pipeline)
      vkDestroyPipeline(m_owner.GetDevice(), m_pipeline, nullptr);
    m_pipeline = new_pipeline;
    m_invalidPipeline = false;
  }
}

void Pipeline::Bind(const vk::CommandBuffer & buffer, VkPipelineBindPoint bindPoint) const
{
  vkCmdBindPipeline(buffer, bindPoint, m_pipeline);
  m_descriptors->Bind(buffer, m_layout, bindPoint);
}

} // namespace RHI::vulkan
