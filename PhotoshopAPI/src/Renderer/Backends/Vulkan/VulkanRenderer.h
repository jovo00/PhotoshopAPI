#pragma once

#include "Macros.h"
#include "VulkanContext.h"
#include "Renderer/Renderer.h"

PSAPI_NAMESPACE_BEGIN

class VulkanRenderer final : public Renderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override = default;

    void init() override;
private:
    std::shared_ptr<VulkanContext> m_Context;
};

PSAPI_NAMESPACE_END