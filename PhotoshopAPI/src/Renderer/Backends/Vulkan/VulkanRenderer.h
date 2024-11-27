#pragma once

#include "Macros.h"
#include "Renderer/Renderer.h"

PSAPI_NAMESPACE_BEGIN

class VulkanRenderer final : public Renderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override = default;

    void init() override;
};

PSAPI_NAMESPACE_END