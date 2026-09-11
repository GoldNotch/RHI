#pragma once

#include <CommandsExecution/CommandBuffer.hpp>
#include <Pipeline/BaseDescriptor.hpp>
#include <Pipeline/DescriptorBufferLayout.hpp>
#include <Pipeline/DescriptorsBuffer.hpp>
#include <Pipeline/PipelineAttachmentsUsage.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <Utils/PipelineBuilder.hpp>
#include <Utils/PipelineLayoutBuilder.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct RenderPass;
namespace details
{
struct CommandBuffer;
}
} // namespace RHI::vulkan

namespace RHI::vulkan
{
using PipelineBindPoint = RenderPass *; // std::variant<RenderPass*, ComputePass*, RayTracePass*>

struct Pipeline final : public IPipeline,
                        public OwnedBy<Context>
{
  explicit Pipeline(Context & ctx);
  virtual ~Pipeline() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IPipeline interface
  virtual void AttachShader(ShaderType type, const SpirV & spirv) override;
  virtual void BindAttachment(uint32_t binding, ShaderAttachmentSlot slot,
                              LayoutIndex inputIndex = LayoutIndex()) override;
  virtual void BindResolver(uint32_t binding, uint32_t resolve_for) override;
  virtual void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type) override;
  virtual void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType) override;
  virtual void DefinePushConstant(uint32_t size, ShaderType shaderStage) override;
  virtual IBufferUniformDescriptor * DeclareUniform(LayoutIndex index,
                                                    ShaderType shaderStage) override;
  virtual ISamplerUniformDescriptor * DeclareSampler(LayoutIndex index,
                                                     ShaderType shaderStage) override;

  virtual void DeclareUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    IBufferUniformDescriptor * outArray[]) override;
  /// Sampler2D / Sampler2DArray uniform
  virtual void DeclareSamplersArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    ISamplerUniformDescriptor * outArray[]) override;

  virtual void SetMeshTopology(MeshTopology topology) noexcept override;

  virtual void EnableDepthTest(bool enabled) noexcept override;
  virtual void SetDepthFunc(CompareOperation op) noexcept override;

public: //ICommandWriter
  void BindToCommandBuffer(details::CommandBuffer & commands, VkPipelineBindPoint bindPoint) const;

public: // IResourceUser
  /// @brief return true if pipeline uses vkCmdPipelineBarrier.
  /// used to detect if RenderPass should have self-dependency
  bool RequireSynchronization() const;
  void CollectResources(std::vector<ResourcePtr> & resources) const;
  void SynchroniseResources(SynchronizationFilter filter, details::CommandBuffer & commands) const;
  const PipelineAttachmentsUsage & GetAttachmentUsageInfo() const & noexcept;

public:
  void Invalidate(RenderPass & renderPass, uint32_t subpassIndex);
  void SetInvalid();

public: // public internal API
  VkPipeline GetPipelineHandle() const noexcept { return m_pipeline; }
  VkPipelineLayout GetPipelineLayoutHandle() const noexcept { return m_pipelineLayout; }
  const DescriptorBufferLayout & GetDescriptorsLayout() const & noexcept;
  DescriptorBufferLayout & GetDescriptorsLayout() & noexcept;
  DescriptorBuffer & GetDescriptorBuffer() & noexcept;
  PipelineBindPoint GetBindPoint() const noexcept;

private:
  PipelineBindPoint m_bindPoint = nullptr; ///< object pipeline is bound to
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;

  utils::PipelineLayoutBuilder m_pipelineLayoutBuilder;
  utils::PipelineBuilder m_pipelineBuilder;

  std::atomic_bool m_invalidPipeline = false;
  bool m_invalidPipelineLayout = false;

  DescriptorBufferLayout m_descriptorsLayout;
  DescriptorBuffer m_descriptorBuffer;

  PipelineAttachmentsUsage m_attachmentsStat;
  std::vector<std::unique_ptr<details::BaseDescriptor>> m_descriptors;
};

} // namespace RHI::vulkan
