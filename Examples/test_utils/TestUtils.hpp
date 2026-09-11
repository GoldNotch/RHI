#pragma once
#include "test_utils_def.h"

#include <cstdio>
#include <filesystem>
#include <string>

#include <RHI.hpp>


// Custom log function used by RHI::Context
void ConsoleLog(RHI::LogMessageStatus status, const std::string & message);


RHI::ITexture * UploadTexture(const char * path, RHI::IContext * ctx, bool with_alpha,
                              bool useMips = false);

RHI::ITexture * UploadLayeredTexture(RHI::IContext * ctx,
                                     const std::vector<std::filesystem::path> & paths,
                                     bool with_alpha, bool useMips = false);

RHI::ITexture * BuildAtlasTexture(RHI::IContext * ctx, uint16_t atlasSize,
                                  const std::vector<std::filesystem::path> & paths, bool with_alpha,
                                  bool useMips = false);

/// @brief reads shader SPIR-V file as binary
/// @param filename - path to file
/// @return vector of bytes
RHI::SpirV ReadSpirV(const std::filesystem::path & path);

std::filesystem::path FromGLSL(const std::filesystem::path & path);

namespace RHI::test_examples
{
struct GlfwInstance final
{
  GlfwInstance();
  ~GlfwInstance();
};

} // namespace RHI::test_examples
