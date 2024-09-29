#pragma once
#include <codecvt>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "../VulkanContext.hpp"

namespace RHI::vulkan::details
{

/// @brief reads shader SPIR-V file as binary
/// @param filename - path to file
/// @return vector of bytes
std::vector<uint32_t> ReadSPIRV(const std::filesystem::path & path)
{
  std::vector<uint32_t> words;
  FILE * f = _wfopen(path.c_str(), L"rb");
  if (f)
  {
    fseek(f, 0, SEEK_END);
    auto len = ftell(f);
    fseek(f, 0, SEEK_SET);
    auto new_size = len % 4 == 0 ? len / 4 : len / 4 + 1;
    words.resize(new_size, 0);
    size_t read_elems = fread(words.data(), sizeof(uint32_t), words.size(), f);
    assert(read_elems > 0);
    fclose(f);
  }
  else
    throw std::runtime_error(std::string("Failed to open file - ") + path.string());
  return words;
}

/// @brief creates shader module in context by filename
/// @param ctx - vulkan context
/// @param filename - path to file
/// @return compiled shader module
vk::ShaderModule BuildShaderModule(const vk::Device & device, const std::filesystem::path & path)
{
  auto code = ReadSPIRV(RHI::details::ResolveShaderExtension(path, apiFolder, shaderExtension));
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size() * sizeof(uint32_t);
  createInfo.pCode = code.data();

  VkShaderModule module;
  if (VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &module);
      result != VK_SUCCESS)
    throw std::runtime_error("failed to create shader module!");
  return vk::ShaderModule(module);
}

} // namespace RHI::vulkan::details
