#include "Renderer.h"
#include "Logger.h"

#include "Backends/Vulkan/VulkanRenderer.h"

PSAPI_NAMESPACE_BEGIN

std::shared_ptr<Renderer> Renderer::create(RendererType type)
{
    switch (type)
    {
    case RendererType::Vulkan:
        return std::make_shared<VulkanRenderer>();
    default:
        return std::make_shared<VulkanRenderer>();
    }
}

PSAPI_NAMESPACE_END