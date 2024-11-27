#pragma once

#include "Macros.h"
#include <memory>

PSAPI_NAMESPACE_BEGIN

enum class RendererType {
    Vulkan
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void init() = 0;

    static std::shared_ptr<Renderer> create(RendererType type);
};

PSAPI_NAMESPACE_END