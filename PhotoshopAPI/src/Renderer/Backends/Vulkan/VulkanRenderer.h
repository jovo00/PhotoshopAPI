#pragma once

#include "Macros.h"
#include "Renderer/Renderer.h"

PSAPI_NAMESPACE_BEGIN

class VulkanContext final : public Renderer {
public:
    VulkanContext() = default;
    ~VulkanContext() override = default;

    void init() override;
};

PSAPI_NAMESPACE_END