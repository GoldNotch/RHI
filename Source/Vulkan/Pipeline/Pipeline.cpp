#include "Pipeline.hpp"

#include <Pipeline/BufferUniform.hpp>
#include <Pipeline/DescriptorBufferLayout.hpp>
#include <Pipeline/InputAttachmentUniform.hpp>
#include <Pipeline/PipelineProcess.hpp>
#include <Pipeline/SamplerArrayUniform.hpp>
#include <Pipeline/SamplerUniform.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <RenderPass/RenderPass.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

Pipeline::Pipeline(Context & ctx)
  : OwnedBy<Context>(ctx)
  , m_descriptorsLayout(ctx)
  , m_descriptorBuffer(ctx)
{
}

Pipeline::~Pipeline()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipelineLayout, nullptr);
}

void Pipeline::AttachShader(ShaderType type, const SpirV & spirv)
{
  m_pipelineBuilder.AttachShader(type, spirv);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void Pipeline::BindAttachment(uint32_t binding, ShaderAttachmentSlot slot,
                              LayoutIndex inputIndex /* = LayoutIndex()*/)
{
  m_attachmentsStat.BindAttachment(slot, binding);
  //  only colored attachment should have colorBlendState
  if (slot & ShaderAttachmentSlot::Color)
  {
    m_pipelineBuilder.OnColorAttachmentHasBound();
    m_invalidPipeline = true;
    m_invalidPipeline.notify_one();
  }
  if (slot & ShaderAttachmentSlot::Input)
  {
    if (!inputIndex.IsValid())
    {
      throw std::runtime_error(
        "Input attachments require valid layout index. Don't forget to declare this uniform in fragment shader");
    }
    const VkDescriptorType type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    // input attachments are fragment-shader-only feature
    // https://vulkan.lunarg.com/doc/view/1.4.335.0/linux/antora/spec/latest/chapters/interfaces.html#interfaces-inputattachment
    GetDescriptorsLayout().DeclareDescriptorsArray(inputIndex, type, RHI::ShaderType::Fragment, 1);
    auto uniform =
      std::make_unique<InputAttachmentUniform>(GetContext(), *this, inputIndex, binding);
    m_descriptors.push_back(std::move(uniform));
  }
}

void Pipeline::BindResolver(uint32_t binding, uint32_t resolve_for)
{
  m_attachmentsStat.BindResolver(binding, resolve_for);
}

void Pipeline::AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type)
{
  m_pipelineBuilder.AddInputBinding(slot, stride, type);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void Pipeline::AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType)
{
  m_pipelineBuilder.AddInputAttribute(binding, location, offset, elemsCount, elemsType);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

IBufferUniformDescriptor * Pipeline::DeclareUniform(LayoutIndex index, ShaderType shaderStage)
{
  IBufferUniformDescriptor * result = nullptr;
  DeclareUniformsArray(index, shaderStage, 1, &result);
  return result;
}

ISamplerUniformDescriptor * Pipeline::DeclareSampler(LayoutIndex index, ShaderType shaderStage)
{
  ISamplerUniformDescriptor * result = nullptr;
  DeclareSamplersArray(index, shaderStage, 1, &result);
  return result;
}

void Pipeline::DeclareUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    IBufferUniformDescriptor * outArray[])
{
  constexpr VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  GetDescriptorsLayout().DeclareDescriptorsArray(index, type, shaderStage, size);
  for (uint32_t i = 0; i < size; ++i)
  {
    auto && newDescriptor = std::make_unique<BufferUniform>(GetContext(), *this, type, index, i);
    outArray[i] = newDescriptor.get();
    m_descriptors.emplace_back(std::move(newDescriptor));
  }
}

void Pipeline::DeclareSamplersArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    ISamplerUniformDescriptor * outArray[])
{
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  GetDescriptorsLayout().DeclareDescriptorsArray(index, type, shaderStage, size);
  for (uint32_t i = 0; i < size; ++i)
  {
    auto descriptor = std::make_unique<SamplerUniform>(GetContext(), *this, type, index, i);
    outArray[i] = descriptor.get();
    m_descriptors.push_back(std::move(descriptor));
  }
}

void Pipeline::DefinePushConstant(uint32_t size, ShaderType shaderStage)
{
  m_pipelineLayoutBuilder.DeclarePushConstant(size, shaderStage);
  m_invalidPipelineLayout = true;
}

void Pipeline::SetMeshTopology(MeshTopology topology) noexcept
{
  m_pipelineBuilder.SetMeshTopology(topology);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void Pipeline::EnableDepthTest(bool enabled) noexcept
{
  m_pipelineBuilder.SetDepthTestEnabled(enabled);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
  SetInvalid();
}

void Pipeline::SetDepthFunc(CompareOperation op) noexcept
{
  m_pipelineBuilder.SetDepthTestCompareOperator(op);
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
}

void Pipeline::BindToCommandBuffer(details::CommandBuffer & commands,
                                   VkPipelineBindPoint bindPoint) const
{
  assert(!!m_pipeline);
  commands.PushCommand(vkCmdBindPipeline, bindPoint, m_pipeline);
  auto && sets = m_descriptorBuffer.GetSets();
  if (!sets.empty())
  {
    for (auto && descriptor : m_descriptors)
      descriptor->UpdateDescriptorSet(sets);
    commands.PushCommand(vkCmdBindDescriptorSets, bindPoint, m_pipelineLayout, 0,
                         static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
  }
}

void Pipeline::CollectResources(std::vector<ResourcePtr> & resources) const
{
  // collect descriptors and uniforms
  for (auto && uniformPtr : m_descriptors)
    uniformPtr->CollectResources(resources);
}

void Pipeline::SynchroniseResources(details::CommandBuffer & commands) const
{
  // collect descriptors and uniforms
  for (auto && uniformPtr : m_descriptors)
    uniformPtr->SynchroniseResources(commands);
}

const PipelineAttachmentsUsage & Pipeline::GetAttachmentUsageInfo() const & noexcept
{
  return m_attachmentsStat;
}

void Pipeline::Invalidate(RenderPass & renderPass, uint32_t subpassIndex)
{
  for (auto && uniformPtr : m_descriptors)
    uniformPtr->Invalidate();
  m_descriptorsLayout.Invalidate();
  m_descriptorBuffer.Invalidate(GetDescriptorsLayout());
  if (m_invalidPipelineLayout || !m_pipelineLayout)
  {
    auto new_layout = m_pipelineLayoutBuilder.Make(GetContext().GetGpuConnection().GetDevice(),
                                                   GetDescriptorsLayout().GetLayouts());
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipelineLayout, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkPipelineLayout({}) has been rebuilt - {}",
                     static_cast<void *>(m_pipelineLayout), static_cast<void *>(new_layout));
    m_pipelineLayout = new_layout;
    m_invalidPipelineLayout = false;
    m_invalidPipeline = true;
  }

  if (m_invalidPipeline || !m_pipeline || m_bindPoint != &renderPass)
  {
    m_pipelineBuilder.SetSamplesCount(renderPass.GetFramebuffer().CalcSamplesCount());
    auto new_pipeline = m_pipelineBuilder.Make(GetContext().GetGpuConnection().GetDevice(),
                                               renderPass.GetHandle(), subpassIndex,
                                               m_pipelineLayout);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_pipeline, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG,
                     "Graphic VkPipeline({}) has been rebuilt - {}",
                     static_cast<void *>(m_pipeline), static_cast<void *>(new_pipeline));
    m_pipeline = new_pipeline;
    m_bindPoint = &renderPass;
    m_invalidPipeline = false;
    m_invalidPipeline.notify_one();
  }
}

void Pipeline::SetInvalid()
{
  m_invalidPipeline = true;
  m_invalidPipeline.notify_one();
  m_invalidPipelineLayout = true;
}

const DescriptorBufferLayout & Pipeline::GetDescriptorsLayout() const & noexcept
{
  return m_descriptorsLayout;
}

DescriptorBufferLayout & Pipeline::GetDescriptorsLayout() & noexcept
{
  return m_descriptorsLayout;
}

DescriptorBuffer & Pipeline::GetDescriptorBuffer() & noexcept
{
  return m_descriptorBuffer;
}

PipelineBindPoint Pipeline::GetBindPoint() const noexcept
{
  return m_bindPoint;
}

} // namespace RHI::vulkan
