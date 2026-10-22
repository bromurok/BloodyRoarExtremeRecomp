#include "gpu/backend.h"
#include "app/app.h"
#include "core/log.h"

namespace gpu {

std::unique_ptr<Backend> CreateBackend() {
    const auto& s = app::Get();
    if (s.renderer == "vulkan") {
        LOGR("gpu: requested Vulkan backend");
        auto vk = CreateVulkanBackend();
        if (vk) {
            LOGR("gpu: Vulkan backend created successfully");
            return vk;
        }
        LOGR("gpu: Vulkan backend initialization failed, falling back to Direct3D 12");
    }
    LOGR("gpu: using Direct3D 12 backend");
    return CreateD3D12Backend();
}

}  // namespace gpu
