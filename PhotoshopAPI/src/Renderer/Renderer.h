#pragma once

#include "Macros.h"
#include <memory>

PSAPI_NAMESPACE_BEGIN


class RendererContext {
public:
    RendererContext() = default;
    virtual ~RendererContext() = default;

    virtual void init() = 0;

    static std::shared_ptr<RendererContext> create();
};

PSAPI_NAMESPACE_END