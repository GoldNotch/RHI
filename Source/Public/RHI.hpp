#pragma once
#include "RHI_def.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <Descriptors.hpp>
#include <Images.hpp>

namespace RHI
{

/// @brief Operation System handle
using ExternalHandle = void *;
/// @brief Internal handle (may be vulkan/DirectX/Metal object)
using InternalObjectHandle = void *;

enum class LogMessageStatus : uint8_t
{
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_DEBUG
};

typedef void (*LoggingFunc)(LogMessageStatus, const std::string &);


/// @brief Generic settings to create a surface
struct SurfaceConfig
{
  /// Generic window Handle (HWND Handle in Windows, X11Window handle in Linux)
  ExternalHandle hWnd;
  /// Additional window handle (HINSTANCE handle in Windows, Display in Linux)
  ExternalHandle hInstance;
};

/// @brief How many buffers can be in one attachment
enum class RenderBuffering : uint8_t
{
  Single = 1,
  Double = 2,
  Triple = 3
};

/// @brief requirements for GPU selector
struct GpuTraits
{
  /// desired name of GPU
  std::optional<std::string> name;
  /// GPU must have presentation
  bool require_presentation = false;
  /// GPU must support geometry shaders
  bool require_geometry_shaders = false;

  // add new flags or requirenets if you need it
};

/// @brief types of shader which can be attached to pipeline
enum class ShaderType : uint8_t
{
  Vertex = utils::bit<uint8_t>(0),
  TessellationControl = utils::bit<uint8_t>(1),
  TessellationEvaluation = utils::bit<uint8_t>(2),
  Geometry = utils::bit<uint8_t>(3),
  Fragment = utils::bit<uint8_t>(4),
  Compute = utils::bit<uint8_t>(5),
};
DECLARE_BIT_OPERATIONS_FOR_ENUM(ShaderType)

/// @brief a way to connect and interpret vertices in VertexData
enum class MeshTopology : uint8_t
{
  Point,        ///< every vertex is point
  Line,         ///< vertices grouped by 2 and draws as line
  LineStrip,    ///< vertices grouped by 2 and draws as line
  Triangle,     ///< vertices grouped by 3 and draws as triangle
  TriangleFan,  ///< vertices grouped by 3 and draws as triangle
  TriangleStrip ///< vertices grouped by 3 and draws as triangle
};

enum class PolygonMode : uint8_t
{
  Fill, ///< Fill polygon by color
  Line, ///< Draw only border of polygon
  Point ///< Draw polygon as point
};

/// @brief A way to define which polygon is front and which is back.
enum class FrontFace : uint8_t
{
  CW, //clockwise
  CCW // counter clockwise
};

/// @brief modes for culling
enum class CullingMode : uint8_t
{
  None,        ///< Culling mode is disabled
  FrontFace,   ///< front faces will be culled
  BackFace,    ///< back faces will be culled
  FrontAndBack ///< front and back polygons will be culled
};

/// @brief binary operation used in blending. It's like operation(source_factor * source_color, dest_factor * dest_color)
enum class BlendOperation : uint8_t
{
  Add,              // src.color + dst.color
  Subtract,         // src.color - dst.color
  ReversedSubtract, //dst.color - src.color
  Min,              // min(src.color, dst.color)
  Max               // max(src.color, dst.color)
};

/// @brief factor, which will be multiplied on color in blending
enum class BlendFactor : uint8_t
{
  Zero, ///<
  One,
  SrcColor,
  OneMinusSrcColor,
  DstColor,
  OneMinusDstColor,
  SrcAlpha,
  OneMinusSrcAlpha,
  DstAlpha,
  OneMinusDstAlpha,
  ConstantColor,
  OneMinusConstantColor,
  ConstantAlpha,
  OneMinusConstantAlpha,
  SrcAlphaSaturate,
  Src1Color,
  OneMinusSrc1Color,
  Src1Alpha,
  OneMinusSrc1Alpha,
};

enum class CompareOperation
{
  Never = 0,
  Less = 1,
  Equal = 2,
  LessOrEqual = 3,
  Greater = 4,
  NotEqual = 5,
  GreaterOrEqual = 6,
  Always = 7
};

/// @brief types of command buffers
enum class CommandBufferType : uint8_t
{
  Executable, ///< executable in GPU
  ThreadLocal ///< filled in separate thread
};

/// @brief enum to define usage of BufferGPU
enum BufferGPUUsage
{
  VertexBuffer = 1,
  IndexBuffer = 2,
  UniformBuffer = 4,
  StorageBuffer = 8,
  TransformFeedbackBuffer = 16,
  IndirectBuffer = 32
};

/// @brief defines what input attribute is used for. For instance or for each vertex
enum class InputBindingType : uint8_t
{
  VertexData,  ///< input attribute will be associated with vertex
  InstanceData ///< input attribute will be associeated with instance
};

/// @brief types for input attributes
enum class InputAttributeElementType : uint8_t
{
  FLOAT,  ///< Input attribute will be interpreted in shader as float
  DOUBLE, ///< Input attribute will be interpreted in shader as double
  UINT,   ///< Input attribute will be interpreted in shader as unsigned int
  SINT    ///< Input attribute will be interpreted in shader as signed int
};

/// @brief data type for vertex indices
enum class IndexType : uint8_t
{
  UINT8,  ///< indices will be interpreted in driver as uint8_t*
  UINT16, ///< indices will be interpreted in driver as uint16_t*
  UINT32  ///< indices will be interpreted in driver as uint32_t*
};


//----------------- Images ---------------------

struct IBufferGPU;
struct ITexture;
struct IAttachment;

//TODO: Remove this interface
struct IInvalidable
{
  virtual ~IInvalidable() = default;
  virtual void SetInvalid() = 0;
  virtual void Invalidate() = 0;
};

struct IAwaitable
{
  virtual ~IAwaitable() = default;
  /// @brief Wait for process completed
  virtual bool Wait() noexcept = 0;
  /// @brief Checks if task is done
  virtual bool IsReady() const noexcept = 0;
};

using SpirV = std::vector<uint32_t>;

struct IPipelineProcess
{
  virtual ~IPipelineProcess() = default;
  /// @brief draw vertices command (analog glDrawArrays)
  virtual void DrawVertices(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0,
                            uint32_t firstInstance = 0) = 0;
  /// @brief draw vertices with indieces (analog glDrawElements)
  virtual void DrawIndexedVertices(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                   uint32_t firstInstance = 0) = 0;
  /// @brief Set viewport command
  virtual void SetViewport(float width, float height) = 0;
  /// @brief Set scissor command
  virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
  /// @brief binds buffer as input attribute data
  virtual void BindVertexBuffer(uint32_t binding, IBufferGPU * buffer, uint32_t offset = 0) = 0;
  /// @brief binds buffer as index buffer
  virtual void BindIndexBuffer(IBufferGPU * buffer, IndexType type, uint32_t offset = 0) = 0;
  virtual void PushConstant(const void * data, size_t size) = 0;
};
using PipelineProcessPtr = std::shared_ptr<IPipelineProcess>;


/// @brief SubpassConfiguration is container for rendering state settings (like shaders, input attributes, uniforms, etc).
/// It has two modes: editing and drawing. In editing mode you can change any settings (attach shaders, uniforms, set viewport, etc).
/// After editing you must call Invalidate(), it rebuilds internal objects and applyies new configuration.
/// After invalidate you can bind it to CommandBuffer and draw.
struct IPipeline
{
  virtual ~IPipeline() = default;
  // General static settings
  /// @brief attach shader to pipeline
  virtual void AttachShader(ShaderType type, const SpirV & spirv) = 0;
  virtual void BindAttachment(uint32_t binding, ShaderAttachmentSlot slot,
                              LayoutIndex inputIndex = LayoutIndex()) = 0;
  virtual void BindResolver(uint32_t binding, uint32_t resolve_for) = 0;

  virtual void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type) = 0;
  virtual void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType) = 0;
  virtual void DefinePushConstant(uint32_t size, ShaderType shaderStage) = 0;

  virtual IBufferUniformDescriptor * DeclareUniform(LayoutIndex index, ShaderType shaderStage) = 0;
  /// Sampler2D / Sampler2DArray uniform
  virtual ISamplerUniformDescriptor * DeclareSampler(LayoutIndex index, ShaderType shaderStage) = 0;

  virtual void DeclareUniformsArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    IBufferUniformDescriptor * outArray[]) = 0;
  /// Sampler2D / Sampler2DArray uniform
  virtual void DeclareSamplersArray(LayoutIndex index, ShaderType shaderStage, uint32_t size,
                                    ISamplerUniformDescriptor * outArray[]) = 0;
  ///// Texture2DArray + sampler uniform
  //virtual ISamplerArrayUniformDescriptor * DeclareSamplersArray(LayoutIndex index,
  //                                                              ShaderType shaderStage,
  //                                                              uint32_t size, uint32_t arrayIndex = 0) = 0;
  virtual void SetMeshTopology(MeshTopology topology) noexcept = 0;

  virtual void EnableDepthTest(bool enabled) noexcept = 0;
  virtual void SetDepthFunc(CompareOperation op) noexcept = 0;
};
using PipelinePtr = std::shared_ptr<IPipeline>;

/// @brief RenderPass is object that can render frames.
struct IFramebuffer
{
  virtual ~IFramebuffer() = default;
  virtual void AddAttachment(uint32_t binding, IAttachment * attachment) = 0;
  virtual void Resize(uint32_t width, uint32_t height) = 0;
  virtual RHI::TexelIndex GetExtent() const = 0;
  virtual void ClearAttachments() noexcept = 0;

  virtual void SetSubpass(
    uint32_t index, PipelinePtr pipeline,
    PipelineProcessPtr process /*TODO: std::initializer_list<int> subpassDeps*/) = 0;
};

/// @brief Generic data buffer in GPU. You can map it on CPU memory and change.
/// After mapping changed data can be sent to GPU. Use Flush method to be sure that data is sent
struct IBufferGPU
{
  using UnmapFunc = std::function<void(char *)>;
  using ScopedPointer = std::unique_ptr<char, UnmapFunc>;

  virtual ~IBufferGPU() = default;
  /// @brief uploads data
  virtual void UploadSync(const void * data, size_t size, size_t offset = 0) = 0;
  virtual std::shared_ptr<IAwaitable> UploadAsync(const void * data, size_t size,
                                                  size_t offset = 0) = 0;
  /// @brief Map buffer into CPU memory.  It will be unmapped in end of scope
  virtual ScopedPointer Map() = 0;
  /// @brief Sends changed buffer after Map to GPU
  virtual void Flush() const noexcept = 0;
  /// @brief checks if buffer is mapped into CPU memory
  virtual bool IsMapped() const noexcept = 0;
  /// @brief Get size of buffer in bytes
  virtual size_t Size() const noexcept = 0;
};

/// Image with mipmaps, compression
struct ITexture
{
  virtual ~ITexture() = default;
  virtual std::shared_ptr<IAwaitable> UploadImage(const UploadImageArgs & args) = 0;
  virtual std::shared_ptr<IAwaitable> DownloadImage(const DownloadImageArgs & args) = 0;
  /// @brief generate mipmaps as declared in TextureDescription
  /// @return future with count of generated mip levels
  virtual std::shared_ptr<IAwaitable> GenerateMipmaps() = 0;

  virtual TextureDescription GetDescription() const noexcept = 0;
  virtual size_t Size() const = 0;
  //virtual void SetSwizzle() = 0;
  virtual void BlitTo(ITexture * texture) = 0;
};

/// swapchained image sequence to attach it to framebuffer
struct IAttachment
{
  virtual ~IAttachment() = default;
  virtual std::shared_ptr<IAwaitable> DownloadImage(const DownloadImageArgs & args) = 0;
  virtual TextureDescription GetDescription() const noexcept = 0;
  virtual size_t Size() const = 0;
  virtual void BlitTo(ITexture * texture) = 0;
  virtual void SetClearValue(float r, float g, float b, float a) = 0;
  virtual void SetClearValue(float depth, uint32_t stencil) = 0;
};

/// @brief Context is a main container for all objects above. It can creates some user-defined objects like buffers, framebuffers, etc
struct IContext
{
  virtual ~IContext() = default;

  virtual void ClearResources() = 0;
  virtual IAwaitable * TransferPass(std::span<const IAwaitable *> commandsToWait = {}) = 0;
  virtual IAwaitable * RenderPass(IFramebuffer * framebuffer,
                                  std::span<const IAwaitable *> commandsToWait = {}) = 0;

  virtual PipelinePtr CreatePipeline() = 0;
  virtual PipelineProcessPtr CreateProcess() = 0;
  virtual IFramebuffer * CreateFramebuffer() = 0;
  virtual void DeleteFramebuffer(IFramebuffer * fbo) = 0;
  virtual IBufferGPU * CreateBuffer(size_t size, BufferGPUUsage usage, bool allowHostAccess) = 0;
  virtual void DeleteBuffer(IBufferGPU * buffer) = 0;
  virtual ITexture * CreateTexture(const TextureDescription & args) = 0;
  virtual void DeleteTexture(ITexture * texture) = 0;

  virtual IAttachment * CreateSurfacedAttachment(const SurfaceConfig & surfaceTraits,
                                                 RenderBuffering buffering) = 0;
  virtual IAttachment * CreateAttachment(RHI::ImageFormat format, const RHI::TextureExtent & extent,
                                         RenderBuffering buffering,
                                         RHI::SamplesCount samplesCount) = 0;
  virtual void DeleteAttachment(IAttachment * attachment) = 0;
};

/// @brief Factory-function to create context
RHI_API std::unique_ptr<IContext> CreateContext(const GpuTraits & gpuTraits,
                                                LoggingFunc loggingFunc = nullptr);

} // namespace RHI
