#pragma once

#include "Macros.h"

#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

PSAPI_NAMESPACE_BEGIN


class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

private:
    bool m_IsInitialized = false;

    VkInstance m_Instance;
    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;

    VkQueue m_ComputeQueue;
    uint32_t m_ComputeQueueFamily;

    VmaAllocator m_Allocator;
};

PSAPI_NAMESPACE_END