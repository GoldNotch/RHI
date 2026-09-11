
#include "Device.hpp"

#include <cassert>
#include <sstream>

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <VulkanContext.hpp>

namespace
{
constexpr uint32_t VulkanAPIVersion = VK_API_VERSION_1_3;
constexpr std::pair<uint32_t, uint32_t> VulkanAPIVersionPair = {1, 3};


VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData)
{
  using SeverityBitFlag = VkDebugUtilsMessageSeverityFlagBitsEXT;
  using SeverityFlags = VkDebugUtilsMessageSeverityFlagBitsEXT;
  using TypeBitFlag = VkDebugUtilsMessageTypeFlagBitsEXT;
  using TypeFlags = VkDebugUtilsMessageTypeFlagsEXT;
  RHI::vulkan::Context * ctx = reinterpret_cast<RHI::vulkan::Context *>(pUserData);

  if (ctx)
  {
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ||
        messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
      ctx->Log(RHI::LogMessageStatus::LOG_ERROR, "{}", std::string_view(pCallbackData->pMessage));
    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
             messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
      ctx->Log(RHI::LogMessageStatus::LOG_WARNING, "{}", std::string_view(pCallbackData->pMessage));
    else
      ctx->Log(RHI::LogMessageStatus::LOG_INFO, "{}", std::string_view(pCallbackData->pMessage));
  }
  return VK_FALSE;
}


vkb::Instance CreateInstance(const char * appName, uint32_t apiVersion, void * debugUserData)
{
  vkb::Instance result;
  vkb::InstanceBuilder builder;
  auto inst_ret = builder
                    .set_app_name(appName)
#ifndef ENABLE_VALIDATION_LAYERS
                    .request_validation_layers(false)
#else
                    .request_validation_layers()
                    //.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
                    .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
                    .add_validation_feature_enable(
                      VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
#endif
                    .set_debug_callback(VulkanDebugCallback)
                    .set_debug_callback_user_data_pointer(debugUserData)
                    .set_minimum_instance_version(apiVersion) // comment if you run with RenderDoc, because it can use vulkan higher 1.0
                    .build();
  if (!inst_ret || !inst_ret.has_value())
  {
    std::stringstream ss;
    ss << "Failed to create Vulkan instance: ";
    for (auto && errMsg : inst_ret.detailed_failure_reasons())
      ss << errMsg << "; ";
    throw std::runtime_error(ss.str());
  }
  else
  {
    result = inst_ret.value();
  }
  return result;
}

vkb::PhysicalDevice SelectPhysicalDevice(vkb::Instance inst, const RHI::GpuTraits & gpuTraits,
                                         const std::pair<uint32_t, uint32_t> & apiVersion)
{
  vkb::PhysicalDeviceSelector selector{inst};
  // init features for vulkan 1.1
  VkPhysicalDeviceVulkan11Features features11{};
  {
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  }
  // init features for vulkan 1.2
  VkPhysicalDeviceVulkan12Features features12{};
  {
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.timelineSemaphore = VK_TRUE;
  }
  // init features for vulkan 1.3
  VkPhysicalDeviceVulkan13Features features13{};
  {
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.synchronization2 = VK_TRUE;
  }
  // init features for vulkan 1.4
  VkPhysicalDeviceVulkan14Features features14{};
  {
    features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
  }
  // init other features
  VkPhysicalDeviceFeatures features{};
  {
    if (gpuTraits.require_geometry_shaders)
      features.geometryShader = VK_TRUE;
    if (gpuTraits.name.has_value())
      selector.set_name(*gpuTraits.name);
    if (gpuTraits.require_presentation)
      selector.defer_surface_initialization();
    features.sampleRateShading = VK_TRUE;
  }
  auto phys_ret =
    selector.set_required_features(features)
      .set_required_features_11(features11)
      .set_required_features_12(features12)
      .set_required_features_13(features13)
      .set_required_features_14(features14)
      //.set_minimum_version(apiVersion.first, apiVersion.second) // RenderDoc doesn't work with it
      .select();

  if (!phys_ret)
  {
    std::stringstream ss;
    ss << "Failed to select Vulkan Physical Device: ";
    for (auto && errMsg : phys_ret.detailed_failure_reasons())
      ss << errMsg << "; ";
    throw std::runtime_error(ss.str());
  }
  return phys_ret.value();
}

struct DeviceInternal final
{
  explicit DeviceInternal(const RHI::GpuTraits & gpuTraits, RHI::vulkan::Context & ctx);
  ~DeviceInternal();

  const vkb::Instance & GetInstance() const & noexcept { return instance; }
  const vkb::Device & GetDevice() const & noexcept { return device; }
  const vkb::PhysicalDevice & GetPhysicalDevice() const & noexcept { return physicalDevice; }
  bool GetQueue(vkb::QueueType type, uint32_t & resultFamily, VkQueue & resultQueue) const noexcept;

private:
  vkb::Instance instance;
  vkb::PhysicalDevice physicalDevice;
  vkb::Device device;
  vkb::DispatchTable dispatchTable;
};

DeviceInternal::DeviceInternal(const RHI::GpuTraits & gpuTraits, RHI::vulkan::Context & ctx)
{
  instance = CreateInstance("appName", VulkanAPIVersion, &ctx);
  auto major = VK_API_VERSION_MAJOR(instance.api_version);
  auto minor = VK_API_VERSION_MINOR(instance.api_version);
  ctx.Log(RHI::LogMessageStatus::LOG_DEBUG, "VkInstance {}.{} ({}) has been created successfully",
          major, minor, static_cast<void *>(instance.instance));

  physicalDevice = SelectPhysicalDevice(instance, gpuTraits, VulkanAPIVersionPair);
  ctx.Log(RHI::LogMessageStatus::LOG_DEBUG,
          "VkPhysicalDevice ({}) has been selected successfully - {}",
          static_cast<void *>(physicalDevice.physical_device), physicalDevice.name);

  vkb::DeviceBuilder device_builder{physicalDevice};
  auto dev_ret = device_builder.build();
  if (!dev_ret)
  {
    std::stringstream ss;
    ss << "Failed to create Vulkan device: ";
    for (auto && errMsg : dev_ret.detailed_failure_reasons())
      ss << errMsg << "; ";
    throw std::runtime_error(ss.str());
  }
  ctx.Log(RHI::LogMessageStatus::LOG_DEBUG, "VkDevice ({}) has been created successfully",
          static_cast<void *>(dev_ret.value().device));
  device = dev_ret.value();
  dispatchTable = device.make_table();
}

DeviceInternal::~DeviceInternal()
{
  vkb::destroy_device(device);
  vkb::destroy_instance(instance);
}

bool DeviceInternal::GetQueue(vkb::QueueType type, uint32_t & resultFamily,
                              VkQueue & resultQueue) const noexcept
{
  auto queue_ret = device.get_queue(type);
  auto familly_index = device.get_queue_index(type);
  if (!queue_ret || !familly_index)
    return false;
  resultFamily = familly_index.value();
  resultQueue = queue_ret.value();
  return true;
}

} // namespace

namespace RHI::vulkan
{

Device::Device(Context & ctx, const GpuTraits & gpuTraits)
  : OwnedBy<Context>(ctx)
{
  static_assert(sizeof(DeviceInternal) <= sizeof(m_privateData));
  auto * privData = new (m_privateData.data()) DeviceInternal(gpuTraits, ctx);

  privData->GetQueue(vkb::QueueType::graphics, m_queues[QueueType::Graphics].first,
                     m_queues[QueueType::Graphics].second);

  if (!privData->GetQueue(vkb::QueueType::transfer, m_queues[QueueType::Transfer].first,
                          m_queues[QueueType::Transfer].second))
    m_queues[QueueType::Transfer] = m_queues[QueueType::Graphics];

  if (!privData->GetQueue(vkb::QueueType::compute, m_queues[QueueType::Compute].first,
                          m_queues[QueueType::Compute].second))
    m_queues[QueueType::Compute] = m_queues[QueueType::Graphics];

  if (!privData->GetQueue(vkb::QueueType::present, m_queues[QueueType::Present].first,
                          m_queues[QueueType::Present].second))
    m_queues[QueueType::Present] = m_queues[QueueType::Graphics];

  auto extensions = privData->GetPhysicalDevice().get_available_extensions();
  m_availableExtensions.insert(extensions.begin(), extensions.end());
}

Device::~Device()
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  if (privData)
    privData->~DeviceInternal();
}

VkDevice Device::GetDevice() const noexcept
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetDevice().device : VK_NULL_HANDLE;
}

VkInstance Device::GetInstance() const noexcept
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetInstance().instance : VK_NULL_HANDLE;
}

VkPhysicalDevice Device::GetGPU() const noexcept
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetPhysicalDevice().physical_device : VK_NULL_HANDLE;
}

const VkPhysicalDeviceProperties & Device::GetGpuProperties() const & noexcept
{
  static VkPhysicalDeviceProperties s_nullProperties{};
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetPhysicalDevice().properties : s_nullProperties;
}

std::pair<uint32_t, VkQueue> Device::GetQueue(QueueType type) const
{
  return m_queues[type];
}

uint32_t Device::GetVulkanVersion() const noexcept
{
  auto * privData = reinterpret_cast<const DeviceInternal *>(m_privateData.data());
  return privData ? privData->GetInstance().api_version : 0;
}

bool Device::CheckExtension(std::string_view extension) const noexcept
{
   return m_availableExtensions.contains(std::string(extension));
}

} // namespace RHI::vulkan
