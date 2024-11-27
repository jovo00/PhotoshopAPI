#include "VulkanRenderer.h"

#include "VulkanUtil.h"

PSAPI_NAMESPACE_BEGIN

void VulkanRenderer::init()
{
    m_Context = std::make_shared<VulkanContext>();

    VK_LOG("Renderer initialized");
}

PSAPI_NAMESPACE_END
