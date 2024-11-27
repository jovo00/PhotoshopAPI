#pragma once

#include <cassert>
#include <vulkan/vulkan.h>

#include "Logger.h"


#define VK_LOG(...) PSAPI_LOG("Vulkan", __VA_ARGS__)
#define VK_LOG_ERROR(...) PSAPI_LOG_ERROR("Vulkan Error", __VA_ARGS__)

// Vulkan Error Check
#define VK_CHECK(x)\
do\
{\
VkResult err = x;\
if (err)\
{\
VK_LOG_ERROR(string_VkResult(err));\
abort();\
}\
} while (0)


// Vulkan Assert
#define VK_ASSERT(x, ...) \
do \
{ \
if (!(x)) \
{ \
VK_LOG_ERROR(__VA_ARGS__); \
assert(x); \
} \
} while (0)

PSAPI_NAMESPACE_BEGIN

// Vulkan Debug Callback
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

PSAPI_NAMESPACE_END