#include "VulkanUtil.h"

PSAPI_NAMESPACE_BEGIN

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    // Ignore additional debug data for now, maybe implement later
    (void)messageSeverity;
    (void)messageType;
    (void)pUserData;

    VK_LOG("ValidationLayer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}

PSAPI_NAMESPACE_END
