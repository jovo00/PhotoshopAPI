#include "Renderer/Renderer.h"

int main()
{
    using namespace NAMESPACE_PSAPI;

    const auto renderer = Renderer::create(RendererType::Vulkan);
    renderer->init();

}
