#pragma once
#include <memory>
#include "gpu/backend.h"

namespace gpu {

std::unique_ptr<Backend> CreateVulkanBackend();

}  // namespace gpu
