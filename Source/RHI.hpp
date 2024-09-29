#pragma once
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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


#define BIT(x) (1 << (x))
/// @brief types of shader which can be attached to pipeline
enum class ShaderType : uint8_t
{
  Vertex = BIT(0),
  TessellationControl = BIT(1),
  TessellationEvaluation = BIT(2),
  Geometry = BIT(3),
  Fragment = BIT(4),
  Compute = BIT(5),
};

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

/// @brief
enum class ShaderImageSlot : uint8_t
{
  Color,
  DepthStencil,
  Input,
  TOTAL
};

/// @brief types of command buffers
enum class CommandBufferType : uint8_t
{
  Executable, ///< executable in GPU
  ThreadLocal ///< filled in separate thread
};

/// @brief enum to define usage of BufferGPU
enum class BufferGPUUsage : uint8_t
{
  VertexBuffer,
  IndexBuffer,
  UniformBuffer,
  StorageBuffer,
  TransformFeedbackBuffer,
  IndirectBuffer
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

struct ICommandBuffer;
struct IBufferGPU;

/// @brief Pipeline is container for rendering state settings (like shaders, input attributes, uniforms, etc).
/// It has two modes: editing and drawing. In editing mode you can change any settings (attach shaders, uniforms, set viewport, etc).
/// After editing you must call Invalidate(), it rebuilds internal objects and applyies new configuration.
/// After invalidate you can bind it to CommandBuffer and draw.
struct IPipeline
{
  virtual ~IPipeline() = default;
  // General static settings
  /// @brief attach shader to pipeline
  virtual void AttachShader(ShaderType type, const wchar_t * path) = 0;
  virtual void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type) = 0;
  virtual void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType) = 0;

  virtual IBufferGPU * DeclareUniform(const char * name, uint32_t binding, ShaderType shaderStage,
                                      uint32_t size) = 0;

  /// @brief Rebuild object after settings were changed
  virtual void Invalidate() = 0;
  /// @brief Get subpass index
  virtual uint32_t GetSubpass() const = 0;
};

/// @brief Framebuffer is a set of images to render
struct IFramebuffer
{
  virtual ~IFramebuffer() = default;
  /// @brief Swapchain calls that on resize
  virtual void SetExtent(uint32_t width, uint32_t height) = 0;

  /// @brief Rebuild object after settings were changed
  virtual void Invalidate() = 0;
  virtual InternalObjectHandle GetRenderPass() const = 0;
  virtual InternalObjectHandle GetHandle() const = 0;
};

/// @brief Swapchain is object for rendering frames and present them onto surface (OS window)
/// Swapchain contains final command buffer which will be submitted to GPU for drawing.
/// You can fill it with another command buffers (which can be filled in parallel)
struct ISwapchain
{
  virtual ~ISwapchain() = default;
  /// @brief Rebuild object after settings were changed
  virtual void Invalidate() = 0;
  /// @brief begin frame rendering. Returns buffer for drawing commands
  virtual ICommandBuffer * BeginFrame(std::array<float, 4> clearColorValue = {0.0f, 0.0f, 0.0f,
                                                                              0.0f}) = 0;
  /// @brief End frame rendering. Uploads commands on GPU
  virtual void EndFrame() = 0;
  /// @brief Get current extent (screen size)
  virtual std::pair<uint32_t, uint32_t> GetExtent() const = 0;
  /// @brief Get Default framebuffer
  virtual const IFramebuffer & GetDefaultFramebuffer() const & noexcept = 0;
  /// @brief Create thread local command buffer
  virtual std::unique_ptr<ICommandBuffer> CreateCommandBuffer() const = 0;
};

/// @brief buffer for GPU commands (like bindVertexBuffer or draw something)
struct ICommandBuffer
{
  virtual ~ICommandBuffer() = default;

  /// @brief draw vertices command (analog glDrawArrays)
  virtual void DrawVertices(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0,
                            uint32_t firstInstance = 0) const = 0;
  /// @brief draw vertices with indieces (analog glDrawElements)
  virtual void DrawIndexedVertices(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                   uint32_t firstInstance = 0) const = 0;
  /// @brief Set viewport command
  virtual void SetViewport(float width, float height) = 0;
  /// @brief Set scissor command
  virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
  /// @brief binds buffer as input attribute data
  virtual void BindVertexBuffer(uint32_t binding, const IBufferGPU & buffer,
                                uint32_t offset = 0) = 0;
  /// @brief binds buffer as index buffer
  virtual void BindIndexBuffer(const IBufferGPU & buffer, IndexType type, uint32_t offset = 0) = 0;

  /// @brief Clears all commands from buffer
  virtual void Reset() const = 0;
  /// @brief Enable writing mode (only for thread local buffers). Also binds framebuffer and pipeline
  virtual void BeginWriting(const IFramebuffer & framebuffer, const IPipeline & pipeline) = 0;
  /// @brief disables writing mode
  virtual void EndWriting() = 0;
  /// @brief take commands from another buffer
  virtual void AddCommands(const ICommandBuffer & buffer) const = 0;
  /// @brief get type of buffer
  virtual CommandBufferType GetType() const noexcept = 0;
};

/// @brief Generic data buffer in GPU. You can map it on CPU memory and change.
/// After mapping changed data can be sent to GPU. Use Flush method to be sure that data is sent
struct IBufferGPU
{
  using UnmapFunc = std::function<void(void *)>;
  using ScopedPointer = std::unique_ptr<void, UnmapFunc>;

  virtual ~IBufferGPU() = default;
  /// @brief Map buffer into CPU memory.  It will be unmapped in end of scope
  virtual ScopedPointer Map() = 0;
  /// @brief Sends changed buffer after Map to GPU
  virtual void Flush() const noexcept = 0;
  /// @brief checks if buffer is mapped into CPU memory
  virtual bool IsMapped() const noexcept = 0;
  /// @brief Get size of buffer in bytes
  virtual size_t Size() const noexcept = 0;
  /// @brief internal handle (for internal usage)
  virtual InternalObjectHandle GetHandle() const noexcept = 0;
};


/// @brief Context is a main container for all objects above. It can creates some user-defined objects like buffers, framebuffers, etc
struct IContext
{
  virtual ~IContext() = default;
  virtual ISwapchain & GetSwapchain() & noexcept = 0;
  virtual const ISwapchain & GetSwapchain() const & noexcept = 0;
  /// @brief wait for GPU is idle
  virtual void WaitForIdle() const = 0;

  /// @brief create offscreen framebuffer
  virtual std::unique_ptr<IFramebuffer> CreateFramebuffer() const = 0;
  /// @brief create new pipeline
  virtual std::unique_ptr<IPipeline> CreatePipeline(const IFramebuffer & framebuffer,
                                                    uint32_t subpassIndex) const = 0;

  /// @brief creates BufferGPU
  virtual std::unique_ptr<IBufferGPU> AllocBuffer(size_t size, BufferGPUUsage usage,
                                                  bool mapped = false) const = 0;
};

/// @brief Factory-function to create context
std::unique_ptr<IContext> CreateContext(const SurfaceConfig & config,
                                        LoggingFunc loggingFunc = nullptr);

} // namespace RHI


constexpr inline RHI::ShaderType operator|(RHI::ShaderType t1, RHI::ShaderType t2)
{
  return static_cast<RHI::ShaderType>(static_cast<uint8_t>(t1) | static_cast<uint8_t>(t2));
}

constexpr inline RHI::ShaderType operator&(RHI::ShaderType t1, RHI::ShaderType t2)
{
  return static_cast<RHI::ShaderType>(static_cast<uint8_t>(t1) & static_cast<uint8_t>(t2));
}
