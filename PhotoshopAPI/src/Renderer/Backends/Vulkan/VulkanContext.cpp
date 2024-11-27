#include "VulkanContext.h"
#include "VulkanUtil.h"
#include <vector>

#include <vulkan/vk_enum_string_helper.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

PSAPI_NAMESPACE_BEGIN


VulkanContext::VulkanContext()
    : m_PhysicalDevice(VK_NULL_HANDLE)
{
    // Create a Vulkan Instance
    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "PhotoshopAPI";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "PhotoshopAPI";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    std::vector<const char*> instanceExtensions = {};

    // Validation Layers
#ifndef NDEBUG
    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
#else
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = nullptr;
    instanceCreateInfo.pNext = nullptr;
#endif

    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance));

    // Get Physical Device
    uint32_t deviceCount = 0;

    VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr));
    VK_ASSERT(deviceCount > 0, "No GPU found");

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, physicalDevices.data()));

    // Find a Physical Device
    for (VkPhysicalDevice physicalDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

        // TODO: Check if the GPU supports everything we need

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            m_PhysicalDevice = physicalDevice;
            break;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        m_PhysicalDevice = physicalDevices[0];
    }

    VK_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "No suitable GPU found");

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);

    VK_LOG("Using %s", deviceProperties.deviceName);

    // Get queue family
    const float defaultQueuePriority(0.0f);
    uint32_t queueFamilyCount = 0;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
    {
        // We should get additional families for transfer etc.
        // For simplicity, we only use a single queue family
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            m_ComputeQueueFamily = i;
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = i;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
            break;
        }
    }

    // Setup features we need
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Features = {};
    synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    synchronization2Features.synchronization2 = VK_TRUE;

    bufferDeviceAddressFeatures.pNext = &synchronization2Features;

    //// Get the features of the physical device
    // VkPhysicalDeviceFeatures deviceFeatures;
    // vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeatures);

    // Create logical device
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &bufferDeviceAddressFeatures;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    std::vector<const char *> deviceExtensions = {};

    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    // deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));

    // Create the queue
    vkGetDeviceQueue(m_Device, m_ComputeQueueFamily, 0, &m_ComputeQueue);

    // initialize memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_PhysicalDevice;
    allocatorInfo.device = m_Device;
    allocatorInfo.instance = m_Instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &m_Allocator);

    m_IsInitialized = true;
}

VulkanContext::~VulkanContext()
{
    if (!m_IsInitialized)
        return;

    // Wait for GPU finishing all the work
    vkDeviceWaitIdle(m_Device);

    // Delete Vulkan resources
    vmaDestroyAllocator(m_Allocator);
    vkDestroyDevice(m_Device, nullptr);
    vkDestroyInstance(m_Instance, nullptr);

    VK_LOG("Deleted all Vulkan objects");

    m_IsInitialized = false;
}

PSAPI_NAMESPACE_END
