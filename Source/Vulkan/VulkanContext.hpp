#pragma once
#include <CommandsExecution/DoubleBufferedSubmitter.hpp>
#include <Device.hpp>
#include <GarbageCollector.hpp>
#include <Memory/MemoryAllocator.hpp>
#include <Private/ObjectsTable.hpp>
#include <RHI.hpp>
#include <TransferPass/Transferer.hpp>

namespace RHI::vulkan
{

/// @brief context is object contains vulkan logical device. Also it provides access to vulkan functions
///			If rendering system uses several GPUs, you should create one context for each physical device
struct Context final : public IContext
{
  /// @brief constructor
  explicit Context(const GpuTraits & gpuTraits, LoggingFunc log);
  RESTRICTED_COPY(Context);

public: // IContext interface
  virtual IAttachment * CreateSurfacedAttachment(const SurfaceConfig & surfaceTraits,
                                                 RenderBuffering buffering) override;
  virtual PipelinePtr CreatePipeline() override;
  virtual PipelineProcessPtr CreateProcess() override;
  virtual IFramebuffer * CreateFramebuffer() override;
  virtual void DeleteFramebuffer(IFramebuffer * fbo) override;
  virtual IBufferGPU * CreateBuffer(size_t size, BufferGPUUsage usage,
                                    bool allowHostAccess) override;
  virtual void DeleteBuffer(IBufferGPU * buffer) override;
  virtual ITexture * CreateTexture(const TextureDescription & args) override;
  virtual void DeleteTexture(ITexture * texture) override;
  virtual IAttachment * CreateAttachment(RHI::ImageFormat format, const RHI::TextureExtent & extent,
                                         RenderBuffering buffering,
                                         RHI::SamplesCount samplesCount) override;
  virtual void DeleteAttachment(IAttachment * attachment) override;

  // --------------- Passes -------------------
  virtual void ClearResources() override; ///< GarbageCollector call
  virtual IAwaitable * TransferPass(std::span<const IAwaitable *> commandsToWait = {}) override;
  virtual IAwaitable * RenderPass(IFramebuffer * framebuffer,
                                  std::span<const IAwaitable *> commandsToWait = {}) override;

public: // RHI-only API
  void WaitForIdle() const noexcept;
  bool IsValid() const noexcept { return m_validatationMark == kValidationMark; }

  const Device & GetGpuConnection() const & noexcept;
  Transferer & GetTransferer(QueueType queue) &;
  memory::MemoryAllocator & GetBuffersAllocator() & noexcept;
  const details::VkObjectsGarbageCollector & GetGarbageCollector() const & noexcept;

  RHI::ITexture * GetNullTexture() const noexcept;

  template<typename... Args>
  void Log(LogMessageStatus status, const std::format_string<Args...> fmt, Args &&... args)
  {
#ifdef RHI_USE_LOG_OUTPUT
    LogImpl(status, std::format(fmt, std::forward<Args>(args)...));
#endif
  }

private:
  void LogImpl(LogMessageStatus status, const std::string & message) const noexcept;

private:
  static constexpr size_t kValidationMark = 0xABCDEF00ABCDEF00;
  size_t m_validatationMark = kValidationMark;
  LoggingFunc m_logFunc;
  Device m_device;
  memory::MemoryAllocator m_allocator;
  details::VkObjectsGarbageCollector m_gc;
  BufferedSubmitter m_graphicSubmitter;
  BufferedSubmitter m_transferSubmitter;
  BufferedSubmitter m_computeSubmitter;
  Transferer m_graphicTransferer;
  Transferer m_transferTransferer;
  Transferer m_computeTransferer;

  RHI::utils::ObjectsTable<IFramebuffer> m_framebuffers;
  RHI::utils::ObjectsTable<IBufferGPU> m_buffers;
  RHI::utils::ObjectsTable<IAttachment> m_attachments;
  RHI::utils::ObjectsTable<ITexture> m_textures;
  ITexture * m_nullTexture;
};

} // namespace RHI::vulkan
