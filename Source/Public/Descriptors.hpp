#pragma once
#include <cstdint>
#include <memory>

namespace RHI
{
struct IBufferGPU;
struct ITexture;

struct LayoutIndex final
{
  uint32_t set = -1;
  uint32_t binding = -1;

  bool operator==(const LayoutIndex & rhs) const noexcept
  {
    return set == rhs.set && binding == rhs.binding;
  }

  inline bool IsValid() const noexcept { return set != -1 && binding != -1; }
};


enum class TextureWrapping : uint8_t
{
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder
};

enum class TextureFilteration : uint8_t
{
  Nearest,
  Linear
};


struct IUniformDescriptor
{
  virtual ~IUniformDescriptor() = default;
  virtual uint32_t GetSet() const noexcept = 0;
  virtual uint32_t GetBinding() const noexcept = 0;
  virtual uint32_t GetArrayIndex() const noexcept = 0;
};

struct ISamplerDescriptor
{
  virtual ~ISamplerDescriptor() = default;
  virtual void SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                           RHI::TextureWrapping wWrap) noexcept = 0;
  virtual void SetFilter(RHI::TextureFilteration minFilter,
                         RHI::TextureFilteration magFilter) noexcept = 0;
};

struct ISamplerUniformDescriptor : public IUniformDescriptor,
                                   public ISamplerDescriptor
{
  virtual void AssignImage(ITexture * texture) = 0;
};

struct ISamplerArrayUniformDescriptor : public IUniformDescriptor,
                                        public ISamplerDescriptor
{
  virtual void AssignImage(uint32_t index, ITexture * texture) = 0;
};

struct IBufferUniformDescriptor : public IUniformDescriptor
{
  virtual void AssignBuffer(IBufferGPU * buffer, size_t offset = 0) = 0;
  virtual bool IsBufferAssigned() const noexcept = 0;
};


} // namespace RHI


namespace std
{
template<>
struct hash<RHI::LayoutIndex>
{
  std::size_t operator()(const RHI::LayoutIndex & x) const
  {
    return std::hash<uint32_t>()(x.set) << 32 | std::hash<uint32_t>()(x.binding);
  }
};

} // namespace std
