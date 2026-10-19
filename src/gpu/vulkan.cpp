// Native Vulkan backend for XboxXBE-Recomp
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <dxgiformat.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_win32.h>

#include "app/app.h"
#include "core/log.h"
#include "gpu/backend.h"
#include "gpu/vulkan.h"
#include "gpu/shaders.h"
#include "gpu/texture.h"
#include "gpu/vertex.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include <wrl/client.h>
#include <unknwn.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-attributes"
#endif
#include <dxcapi.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#ifdef __MINGW32__
template<> inline const GUID &__mingw_uuidof<IDxcUtils>() {
    static const GUID id = { 0x4605c4cb, 0x2019, 0x492a, { 0xad, 0xa4, 0x65, 0xf2, 0x0b, 0xb7, 0xd6, 0x7f } };
    return id;
}
template<> inline const GUID &__mingw_uuidof<IDxcCompiler3>() {
    static const GUID id = { 0x228b4687, 0x5a6a, 0x4730, { 0x90, 0x0c, 0x97, 0x02, 0xb2, 0x20, 0x3f, 0x54 } };
    return id;
}
template<> inline const GUID &__mingw_uuidof<IDxcResult>() {
    static const GUID id = { 0x58346cda, 0xdde7, 0x4497, { 0x94, 0x61, 0x6f, 0x87, 0xaf, 0x5e, 0x06, 0x59 } };
    return id;
}
template<> inline const GUID &__mingw_uuidof<IDxcBlob>() {
    static const GUID id = { 0x8ba5fb08, 0x5195, 0x40e2, { 0xac, 0x58, 0x0d, 0x98, 0x9c, 0x3a, 0x01, 0x02 } };
    return id;
}
template<> inline const GUID &__mingw_uuidof<IDxcBlobUtf8>() {
    static const GUID id = { 0x3da636c9, 0xba71, 0x4024, { 0xa3, 0x01, 0x30, 0xcb, 0xf1, 0x25, 0x30, 0x5b } };
    return id;
}
template<> inline const GUID &__mingw_uuidof<IDxcBlobWide>() {
    static const GUID id = { 0xa3f84eab, 0x0faa, 0x497e, { 0xa3, 0x9c, 0xee, 0x6e, 0xd6, 0x0b, 0x2d, 0x84 } };
    return id;
}
template<> inline const GUID &__mingw_uuidof<IDxcOperationResult>() {
    static const GUID id = { 0xcedb484a, 0xd4e9, 0x445a, { 0xb9, 0x91, 0xca, 0x21, 0xca, 0x57, 0x3d, 0x52 } };
    return id;
}
#endif

namespace gpu {

namespace {

// Dynamic Vulkan loader
#define VK_EXPORTED_FUNCTION(fn) PFN_##fn fn = nullptr;
#define VK_GLOBAL_FUNCTION(fn) PFN_##fn fn = nullptr;
#define VK_INSTANCE_FUNCTION(fn) PFN_##fn fn = nullptr;
#define VK_DEVICE_FUNCTION(fn) PFN_##fn fn = nullptr;

VK_EXPORTED_FUNCTION(vkGetInstanceProcAddr)
VK_GLOBAL_FUNCTION(vkCreateInstance)
VK_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties)

VK_INSTANCE_FUNCTION(vkDestroyInstance)
VK_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties)
VK_INSTANCE_FUNCTION(vkCreateDevice)
VK_INSTANCE_FUNCTION(vkGetDeviceProcAddr)
VK_INSTANCE_FUNCTION(vkDestroySurfaceKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR)
VK_INSTANCE_FUNCTION(vkCreateWin32SurfaceKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFormatProperties)

VK_DEVICE_FUNCTION(vkDestroyDevice)
VK_DEVICE_FUNCTION(vkGetDeviceQueue)
VK_DEVICE_FUNCTION(vkQueueSubmit)
VK_DEVICE_FUNCTION(vkQueueWaitIdle)
VK_DEVICE_FUNCTION(vkDeviceWaitIdle)
VK_DEVICE_FUNCTION(vkAllocateMemory)
VK_DEVICE_FUNCTION(vkFreeMemory)
VK_DEVICE_FUNCTION(vkMapMemory)
VK_DEVICE_FUNCTION(vkUnmapMemory)
VK_DEVICE_FUNCTION(vkCreateBuffer)
VK_DEVICE_FUNCTION(vkDestroyBuffer)
VK_DEVICE_FUNCTION(vkGetBufferMemoryRequirements)
VK_DEVICE_FUNCTION(vkBindBufferMemory)
VK_DEVICE_FUNCTION(vkCreateImage)
VK_DEVICE_FUNCTION(vkDestroyImage)
VK_DEVICE_FUNCTION(vkGetImageMemoryRequirements)
VK_DEVICE_FUNCTION(vkBindImageMemory)
VK_DEVICE_FUNCTION(vkCreateImageView)
VK_DEVICE_FUNCTION(vkDestroyImageView)
VK_DEVICE_FUNCTION(vkCreateShaderModule)
VK_DEVICE_FUNCTION(vkDestroyShaderModule)
VK_DEVICE_FUNCTION(vkCreatePipelineLayout)
VK_DEVICE_FUNCTION(vkDestroyPipelineLayout)
VK_DEVICE_FUNCTION(vkCreateGraphicsPipelines)
VK_DEVICE_FUNCTION(vkDestroyPipeline)
VK_DEVICE_FUNCTION(vkCreatePipelineCache)
VK_DEVICE_FUNCTION(vkDestroyPipelineCache)
VK_DEVICE_FUNCTION(vkGetPipelineCacheData)
VK_DEVICE_FUNCTION(vkCreateRenderPass)
VK_DEVICE_FUNCTION(vkDestroyRenderPass)
VK_DEVICE_FUNCTION(vkCreateFramebuffer)
VK_DEVICE_FUNCTION(vkDestroyFramebuffer)
VK_DEVICE_FUNCTION(vkCreateCommandPool)
VK_DEVICE_FUNCTION(vkDestroyCommandPool)
VK_DEVICE_FUNCTION(vkAllocateCommandBuffers)
VK_DEVICE_FUNCTION(vkFreeCommandBuffers)
VK_DEVICE_FUNCTION(vkResetCommandBuffer)
VK_DEVICE_FUNCTION(vkBeginCommandBuffer)
VK_DEVICE_FUNCTION(vkEndCommandBuffer)
VK_DEVICE_FUNCTION(vkCmdBeginRenderPass)
VK_DEVICE_FUNCTION(vkCmdEndRenderPass)
VK_DEVICE_FUNCTION(vkCmdSetViewport)
VK_DEVICE_FUNCTION(vkCmdSetScissor)
VK_DEVICE_FUNCTION(vkCmdSetBlendConstants)
VK_DEVICE_FUNCTION(vkCmdSetStencilReference)
VK_DEVICE_FUNCTION(vkCmdClearAttachments)
VK_DEVICE_FUNCTION(vkCmdBindPipeline)
VK_DEVICE_FUNCTION(vkCmdBindVertexBuffers)
VK_DEVICE_FUNCTION(vkCmdBindIndexBuffer)
VK_DEVICE_FUNCTION(vkCmdBindDescriptorSets)
VK_DEVICE_FUNCTION(vkCmdDraw)
VK_DEVICE_FUNCTION(vkCmdDrawIndexed)
VK_DEVICE_FUNCTION(vkCmdCopyBufferToImage)
VK_DEVICE_FUNCTION(vkCmdPipelineBarrier)
VK_DEVICE_FUNCTION(vkCmdBlitImage)
VK_DEVICE_FUNCTION(vkCmdClearColorImage)
VK_DEVICE_FUNCTION(vkCmdClearDepthStencilImage)
VK_DEVICE_FUNCTION(vkCreateSemaphore)
VK_DEVICE_FUNCTION(vkDestroySemaphore)
VK_DEVICE_FUNCTION(vkCreateFence)
VK_DEVICE_FUNCTION(vkDestroyFence)
VK_DEVICE_FUNCTION(vkWaitForFences)
VK_DEVICE_FUNCTION(vkResetFences)
VK_DEVICE_FUNCTION(vkCreateSwapchainKHR)
VK_DEVICE_FUNCTION(vkDestroySwapchainKHR)
VK_DEVICE_FUNCTION(vkGetSwapchainImagesKHR)
VK_DEVICE_FUNCTION(vkAcquireNextImageKHR)
VK_DEVICE_FUNCTION(vkQueuePresentKHR)
VK_DEVICE_FUNCTION(vkCreateDescriptorPool)
VK_DEVICE_FUNCTION(vkDestroyDescriptorPool)
VK_DEVICE_FUNCTION(vkCreateDescriptorSetLayout)
VK_DEVICE_FUNCTION(vkDestroyDescriptorSetLayout)
VK_DEVICE_FUNCTION(vkAllocateDescriptorSets)
VK_DEVICE_FUNCTION(vkResetDescriptorPool)
VK_DEVICE_FUNCTION(vkUpdateDescriptorSets)
VK_DEVICE_FUNCTION(vkCreateSampler)
VK_DEVICE_FUNCTION(vkDestroySampler)

HMODULE g_vulkan_dll = nullptr;

bool LoadVulkanGlobal() {
    if (g_vulkan_dll) return true;
    g_vulkan_dll = LoadLibraryW(L"vulkan-1.dll");
    if (!g_vulkan_dll) {
        LOGE("vulkan: failed to load vulkan-1.dll");
        return false;
    }
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(g_vulkan_dll, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr) return false;

    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(nullptr, "vkCreateInstance");
    vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties");
    return vkCreateInstance != nullptr;
}

void LoadVulkanInstance(VkInstance instance) {
#define LOAD_INST(fn) fn = (PFN_##fn)vkGetInstanceProcAddr(instance, #fn);
    LOAD_INST(vkDestroyInstance);
    LOAD_INST(vkEnumeratePhysicalDevices);
    LOAD_INST(vkGetPhysicalDeviceProperties);
    LOAD_INST(vkGetPhysicalDeviceFeatures);
    LOAD_INST(vkGetPhysicalDeviceQueueFamilyProperties);
    LOAD_INST(vkGetPhysicalDeviceMemoryProperties);
    LOAD_INST(vkCreateDevice);
    LOAD_INST(vkGetDeviceProcAddr);
    LOAD_INST(vkDestroySurfaceKHR);
    LOAD_INST(vkGetPhysicalDeviceSurfaceSupportKHR);
    LOAD_INST(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    LOAD_INST(vkGetPhysicalDeviceSurfaceFormatsKHR);
    LOAD_INST(vkGetPhysicalDeviceSurfacePresentModesKHR);
    LOAD_INST(vkCreateWin32SurfaceKHR);
    LOAD_INST(vkGetPhysicalDeviceFormatProperties);
#undef LOAD_INST
}

void LoadVulkanDevice(VkDevice device) {
#define LOAD_DEV(fn) fn = (PFN_##fn)vkGetDeviceProcAddr(device, #fn);
    LOAD_DEV(vkDestroyDevice);
    LOAD_DEV(vkGetDeviceQueue);
    LOAD_DEV(vkQueueSubmit);
    LOAD_DEV(vkQueueWaitIdle);
    LOAD_DEV(vkDeviceWaitIdle);
    LOAD_DEV(vkAllocateMemory);
    LOAD_DEV(vkFreeMemory);
    LOAD_DEV(vkMapMemory);
    LOAD_DEV(vkUnmapMemory);
    LOAD_DEV(vkCreateBuffer);
    LOAD_DEV(vkDestroyBuffer);
    LOAD_DEV(vkGetBufferMemoryRequirements);
    LOAD_DEV(vkBindBufferMemory);
    LOAD_DEV(vkCreateImage);
    LOAD_DEV(vkDestroyImage);
    LOAD_DEV(vkGetImageMemoryRequirements);
    LOAD_DEV(vkBindImageMemory);
    LOAD_DEV(vkCreateImageView);
    LOAD_DEV(vkDestroyImageView);
    LOAD_DEV(vkCreateShaderModule);
    LOAD_DEV(vkDestroyShaderModule);
    LOAD_DEV(vkCreatePipelineLayout);
    LOAD_DEV(vkDestroyPipelineLayout);
    LOAD_DEV(vkCreateGraphicsPipelines);
    LOAD_DEV(vkDestroyPipeline);
    LOAD_DEV(vkCreatePipelineCache);
    LOAD_DEV(vkDestroyPipelineCache);
    LOAD_DEV(vkGetPipelineCacheData);
    LOAD_DEV(vkCreateRenderPass);
    LOAD_DEV(vkDestroyRenderPass);
    LOAD_DEV(vkCreateFramebuffer);
    LOAD_DEV(vkDestroyFramebuffer);
    LOAD_DEV(vkCreateCommandPool);
    LOAD_DEV(vkDestroyCommandPool);
    LOAD_DEV(vkAllocateCommandBuffers);
    LOAD_DEV(vkFreeCommandBuffers);
    LOAD_DEV(vkResetCommandBuffer);
    LOAD_DEV(vkBeginCommandBuffer);
    LOAD_DEV(vkEndCommandBuffer);
    LOAD_DEV(vkCmdBeginRenderPass);
    LOAD_DEV(vkCmdEndRenderPass);
    LOAD_DEV(vkCmdSetViewport);
    LOAD_DEV(vkCmdSetScissor);
    LOAD_DEV(vkCmdSetBlendConstants);
    LOAD_DEV(vkCmdSetStencilReference);
    LOAD_DEV(vkCmdClearAttachments);
    LOAD_DEV(vkCmdBindPipeline);
    LOAD_DEV(vkCmdBindVertexBuffers);
    LOAD_DEV(vkCmdBindIndexBuffer);
    LOAD_DEV(vkCmdBindDescriptorSets);
    LOAD_DEV(vkCmdDraw);
    LOAD_DEV(vkCmdDrawIndexed);
    LOAD_DEV(vkCmdCopyBufferToImage);
    LOAD_DEV(vkCmdPipelineBarrier);
    LOAD_DEV(vkCmdBlitImage);
    LOAD_DEV(vkCmdClearColorImage);
    LOAD_DEV(vkCmdClearDepthStencilImage);
    LOAD_DEV(vkCreateSemaphore);
    LOAD_DEV(vkDestroySemaphore);
    LOAD_DEV(vkCreateFence);
    LOAD_DEV(vkDestroyFence);
    LOAD_DEV(vkWaitForFences);
    LOAD_DEV(vkResetFences);
    LOAD_DEV(vkCreateSwapchainKHR);
    LOAD_DEV(vkDestroySwapchainKHR);
    LOAD_DEV(vkGetSwapchainImagesKHR);
    LOAD_DEV(vkAcquireNextImageKHR);
    LOAD_DEV(vkQueuePresentKHR);
    LOAD_DEV(vkCreateDescriptorPool);
    LOAD_DEV(vkDestroyDescriptorPool);
    LOAD_DEV(vkCreateDescriptorSetLayout);
    LOAD_DEV(vkDestroyDescriptorSetLayout);
    LOAD_DEV(vkAllocateDescriptorSets);
    LOAD_DEV(vkResetDescriptorPool);
    LOAD_DEV(vkUpdateDescriptorSets);
    LOAD_DEV(vkCreateSampler);
    LOAD_DEV(vkDestroySampler);
#undef LOAD_DEV
}

class DxcCompiler {
public:
    DxcCompiler() = default;
    ~DxcCompiler() {
        if (compiler_) compiler_->Release();
        if (utils_) utils_->Release();
        if (dxc_mod_) FreeLibrary(dxc_mod_);
    }

    bool Init() {
        dxc_mod_ = LoadLibraryA("dxcompiler.dll");
        if (!dxc_mod_) {
            dxc_mod_ = LoadLibraryA("C:\\Users\\pres\\Documents\\DescompBloodyVulkan\\BloodyRoar-Recomp\\out\\build\\release\\dxcompiler.dll");
        }
        if (!dxc_mod_) {
            LOGR("vulkan: failed to load dxcompiler.dll");
            return false;
        }

        DxcCreateInstanceProc DxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(dxc_mod_, "DxcCreateInstance");
        if (!DxcCreateInstance) {
            LOGR("vulkan: GetProcAddress DxcCreateInstance failed");
            return false;
        }

        if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils_)))) {
            LOGR("vulkan: DxcCreateInstance CLSID_DxcUtils failed");
            return false;
        }

        if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler_)))) {
            LOGR("vulkan: DxcCreateInstance CLSID_DxcCompiler failed");
            return false;
        }

        LOGR("vulkan: DXC shader compiler initialized with SPIR-V support");
        return true;
    }

    std::vector<uint32_t> Compile(const std::string& hlsl, const wchar_t* entry, const wchar_t* profile) {
        if (!compiler_ || !utils_) return {};

        DxcBuffer source_buffer{};
        source_buffer.Ptr = hlsl.data();
        source_buffer.Size = hlsl.size();
        source_buffer.Encoding = DXC_CP_UTF8;

        std::vector<LPCWSTR> args = {
            L"-E", entry,
            L"-T", profile,
            L"-spirv",
            L"-HV", L"2018",
            L"-fvk-use-dx-layout",
            L"-fvk-use-dx-position-w",
            L"-fvk-b-shift", L"0", L"0",
            L"-fvk-t-shift", L"10", L"0",
            L"-fvk-s-shift", L"20", L"0",
            L"-fspv-target-env=vulkan1.2",
            L"-O0"
        };

        IDxcResult* result = nullptr;
        HRESULT hr = compiler_->Compile(&source_buffer, args.data(), (uint32_t)args.size(), nullptr, IID_PPV_ARGS(&result));
        if (FAILED(hr) || !result) {
            LOGR("vulkan: DXC Compile failed hr=0x%08X", (uint32_t)hr);
            return {};
        }

        HRESULT status = S_OK;
        result->GetStatus(&status);
        if (FAILED(status)) {
            IDxcBlobUtf8* errors = nullptr;
            result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
            if (errors && errors->GetStringLength() > 0) {
                LOGR("vulkan: shader compilation error: %s", errors->GetStringPointer());
            }
            if (errors) errors->Release();
            result->Release();
            return {};
        }

        IDxcBlob* shader_blob = nullptr;
        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader_blob), nullptr);
        result->Release();

        if (!shader_blob) return {};

        const uint32_t* spv_data = static_cast<const uint32_t*>(shader_blob->GetBufferPointer());
        size_t spv_words = shader_blob->GetBufferSize() / sizeof(uint32_t);
        std::vector<uint32_t> spirv(spv_data, spv_data + spv_words);
        shader_blob->Release();
        return spirv;
    }

private:
    HMODULE dxc_mod_ = nullptr;
    IDxcUtils* utils_ = nullptr;
    IDxcCompiler3* compiler_ = nullptr;
};

struct VulkanTarget {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFramebuffer fb = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
    bool depth = false;
    uint32_t gw = 0, gh = 0, w = 0, h = 0;
    uint32_t data = 0;
    bool drawn = false;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint64_t epoch = 0;
};

inline float Bits(uint32_t v) {
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}

inline void Color(uint32_t argb, float* o) {
    o[0] = ((argb >> 16) & 255) / 255.0f;
    o[1] = ((argb >> 8) & 255) / 255.0f;
    o[2] = (argb & 255) / 255.0f;
    o[3] = (argb >> 24) / 255.0f;
}

inline VkCompareOp VkCmp(uint32_t v) {
    if (v >= 0x200 && v <= 0x207) return VkCompareOp(v - 0x200);
    switch (v) {
        case 1: return VK_COMPARE_OP_NEVER;
        case 2: return VK_COMPARE_OP_LESS;
        case 3: return VK_COMPARE_OP_EQUAL;
        case 4: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case 5: return VK_COMPARE_OP_GREATER;
        case 6: return VK_COMPARE_OP_NOT_EQUAL;
        case 7: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case 8: return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_ALWAYS;
    }
}

inline VkBlendFactor VkBlend(uint32_t v, bool alpha) {
    switch (v) {
        case 0: return VK_BLEND_FACTOR_ZERO;
        case 1: return VK_BLEND_FACTOR_ONE;
        case 2: return VK_BLEND_FACTOR_ONE;
        case 3: return alpha ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_SRC_COLOR;
        case 4: return alpha ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case 5: return VK_BLEND_FACTOR_SRC_ALPHA;
        case 6: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case 7: return VK_BLEND_FACTOR_DST_ALPHA;
        case 8: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case 9: return alpha ? VK_BLEND_FACTOR_DST_ALPHA : VK_BLEND_FACTOR_DST_COLOR;
        case 10: return alpha ? VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case 11: return alpha ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case 14: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case 15: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case 0x300: return alpha ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_SRC_COLOR;
        case 0x301: return alpha ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case 0x302: return VK_BLEND_FACTOR_SRC_ALPHA;
        case 0x303: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case 0x304: return VK_BLEND_FACTOR_DST_ALPHA;
        case 0x305: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case 0x306: return alpha ? VK_BLEND_FACTOR_DST_ALPHA : VK_BLEND_FACTOR_DST_COLOR;
        case 0x307: return alpha ? VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case 0x308: return alpha ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        default: return VK_BLEND_FACTOR_ONE;
    }
}

inline VkBlendOp VkBlendOperation(uint32_t v) {
    switch (v) {
        case 1: return VK_BLEND_OP_ADD;
        case 2: return VK_BLEND_OP_SUBTRACT;
        case 3: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case 4: return VK_BLEND_OP_MIN;
        case 5: return VK_BLEND_OP_MAX;
        case 0x800A: return VK_BLEND_OP_SUBTRACT;
        case 0x800B: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case 0x8007: return VK_BLEND_OP_MIN;
        case 0x8008: return VK_BLEND_OP_MAX;
        default: return VK_BLEND_OP_ADD;
    }
}

inline VkFormat DxgiToVkFormat(uint32_t f) {
    switch (f) {
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case DXGI_FORMAT_R32G32B32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
        case DXGI_FORMAT_R32G32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
        case DXGI_FORMAT_R32_FLOAT: return VK_FORMAT_R32_SFLOAT;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_R8G8_UNORM: return VK_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R8_UNORM: return VK_FORMAT_R8_UNORM;
        case DXGI_FORMAT_R16G16B16A16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM;
        case DXGI_FORMAT_R16G16_SNORM: return VK_FORMAT_R16G16_SNORM;
        case DXGI_FORMAT_R16_SNORM: return VK_FORMAT_R16_SNORM;
        case DXGI_FORMAT_R16G16B16A16_SINT: return VK_FORMAT_R16G16B16A16_SINT;
        case DXGI_FORMAT_R16G16_SINT: return VK_FORMAT_R16G16_SINT;
        case DXGI_FORMAT_R16_SINT: return VK_FORMAT_R16_SINT;
        case DXGI_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;
        default: return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
}

inline VkPrimitiveTopology DxTopoToVk(Topology topo, bool strip) {
    if (topo == Topology::Points) return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    if (topo == Topology::Lines) return strip ? VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    return strip ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

inline void MatMul(const float* x, const float* y, float* o) {
    for (int r = 0; r < 4; r++) {
        for (int col = 0; col < 4; col++) {
            o[r * 4 + col] = x[r * 4] * y[col] + x[r * 4 + 1] * y[4 + col] + x[r * 4 + 2] * y[8 + col] +
                             x[r * 4 + 3] * y[12 + col];
        }
    }
}

inline void Transpose(const float* m, float* o) {
    for (int r = 0; r < 4; r++) {
        for (int col = 0; col < 4; col++) o[col * 4 + r] = m[r * 4 + col];
    }
}

inline void InverseTranspose3x3(const float* wv, float* n) {
    float det = wv[0] * (wv[5] * wv[10] - wv[6] * wv[9]) - wv[1] * (wv[4] * wv[10] - wv[6] * wv[8]) +
                wv[2] * (wv[4] * wv[9] - wv[5] * wv[8]);
    float id = det != 0 ? 1.0f / det : 0.0f;
    for (int r = 0; r < 3; r++) {
        for (int col = 0; col < 3; col++) {
            int r1 = (r + 1) % 3;
            int r2 = (r + 2) % 3;
            int c1 = (col + 1) % 3;
            int c2 = (col + 2) % 3;
            n[r * 4 + col] = (wv[r1 * 4 + c1] * wv[r2 * 4 + c2] - wv[r1 * 4 + c2] * wv[r2 * 4 + c1]) * id;
        }
    }
}

struct GuestExtent {
    uint32_t w = 640;
    uint32_t h = 480;
};

inline TexKind TextureKind(const TextureDesc& d) {
    if (d.cube) return kTexCube;
    if (d.depth > 1) return kTex3D;
    return kTex2D;
}

constexpr size_t kTsView = 2;
constexpr size_t kTsProjection = 3;
constexpr size_t kTsTexture0 = 16;
constexpr size_t kTsWorld = 256;

inline void FillTransformConstants(const State& s, GuestExtent size, DrawConstants& c) {
    uint32_t gw = size.w ? size.w : 640;
    uint32_t gh = size.h ? size.h : 480;
    float wv[16];
    float wvp[16];
    MatMul(s.transforms[kTsWorld].data(), s.transforms[kTsView].data(), wv);
    MatMul(wv, s.transforms[kTsProjection].data(), wvp);
    const auto& v = s.viewport;
    float sx = float(v.w) / gw;
    float sy = float(v.h) / gh;
    float ox = (2.0f * v.x + v.w) / gw - 1;
    float oy = 1 - (2.0f * v.y + v.h) / gh;
    float vpm[16] = {sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, v.max_z - v.min_z, 0, ox, oy, v.min_z, 1};
    float full[16];
    MatMul(wvp, vpm, full);
    Transpose(full, c.wvp);
    Transpose(wv, c.wv);
    float n[16] = {};
    InverseTranspose3x3(wv, n);
    Transpose(n, c.wvn);
    for (int t = 0; t < 4; t++) Transpose(s.transforms[kTsTexture0 + size_t(t)].data(), c.tm[t]);
}

constexpr int kLightFloats = 26;
constexpr int kLightDiffuse = 1;
constexpr int kLightSpecular = 5;
constexpr int kLightAmbient = 9;
constexpr int kLightPosition = 13;
constexpr int kLightDirection = 16;
constexpr int kLightRange = 19;
constexpr int kLightFalloff = 20;
constexpr int kLightAttenuation0 = 21;
constexpr int kLightAttenuation1 = 22;
constexpr int kLightAttenuation2 = 23;
constexpr int kLightTheta = 24;
constexpr int kLightPhi = 25;

inline void LightToView(const std::array<uint32_t, kLightFloats>& l, const float* vm, float (&o)[7][4]) {
    float f[kLightFloats];
    std::memcpy(f, l.data(), sizeof(f));
    for (int j = 0; j < 4; j++) {
        o[0][j] = f[kLightDiffuse + j];
        o[1][j] = f[kLightSpecular + j];
        o[2][j] = f[kLightAmbient + j];
    }
    const float* pos = f + kLightPosition;
    const float* dir = f + kLightDirection;
    for (int j = 0; j < 3; j++) {
        o[3][j] = pos[0] * vm[j] + pos[1] * vm[4 + j] + pos[2] * vm[8 + j] + vm[12 + j];
        o[4][j] = dir[0] * vm[j] + dir[1] * vm[4 + j] + dir[2] * vm[8 + j];
    }
    o[3][3] = f[kLightRange];
    o[4][3] = f[kLightFalloff];
    o[5][0] = f[kLightAttenuation0];
    o[5][1] = f[kLightAttenuation1];
    o[5][2] = f[kLightAttenuation2];
    o[6][0] = std::cos(f[kLightTheta] * 0.5f);
    o[6][1] = std::cos(f[kLightPhi] * 0.5f);
}

inline void FillLightConstants(const State& s, const FixedKey& fk, DrawConstants& c) {
    std::memcpy(c.mat, s.material.data(), sizeof(float) * 16);
    c.mat[4][0] = s.material[16];
    Color(s.rs[kRsAmbient], c.amb);
    const float* vm = s.transforms[kTsView].data();
    for (int i = 0; i < 8; i++) {
        if (!((fk.types >> (i * 2)) & 3)) continue;
        LightToView(s.lights[size_t(i)], vm, c.lt[i]);
    }
}

inline void FillSurfaceConstants(const State& s, GuestExtent size, DrawConstants& c) {
    c.surf[0] = float(size.w ? size.w : 640);
    c.surf[1] = float(size.h ? size.h : 480);
    c.surf[2] = 1.0f / (s.zscale != 0.0f ? s.zscale : 1.0f);
    Color(s.rs[kRsFogColor], c.fog_color);
    c.misc[0] = float(SimpleState(s, kMAlphaRef) & 0xFF);
    c.misc[1] = SimpleState(s, kMAlphaTestEnable) ? float(VkCmp(SimpleState(s, kMAlphaFunc))) : 7.0f;
    c.misc[2] = float(VkCmp(s.rs[kRsShadowFunc]));
    c.misc[3] = s.rs[kRsSpecularEnable] ? 1.0f : 0.0f;
}

inline void FillFogConstants(const State& s, bool vertex_program, DrawConstants& c) {
    if (!s.rs[kRsFogEnable]) return;
    if (vertex_program && s.rs[kRsFogTableMode] == 0) {
        c.fog[0] = 1.0f;
        c.fog[1] = 0.0f;
        c.fog[2] = 0.0f;
        c.fog[3] = 3.0f;
        return;
    }
    c.fog[0] = Bits(s.rs[kRsFogStart]);
    c.fog[1] = Bits(s.rs[kRsFogEnd]);
    c.fog[2] = Bits(s.rs[kRsFogDensity]);
    c.fog[3] = float(s.rs[kRsFogTableMode]);
}

inline void FillCombinerConstants(const State& s, DrawConstants& c) {
    for (int i = 0; i < 8; i++) {
        Color(SimpleState(s, kMPsConstant0 + i * 4), c.pc[i * 2]);
        Color(SimpleState(s, kMPsConstant1 + i * 4), c.pc[i * 2 + 1]);
    }
    Color(SimpleState(s, kMPsFinalConstant0), c.pc[16]);
    Color(SimpleState(s, kMPsFinalConstant1), c.pc[17]);
    Color(s.rs[kRsTextureFactor], c.tfactor);
}

inline void FillTextureConstants(const State& s, DrawConstants& c) {
    for (int i = 0; i < 4; i++) {
        if (s.textures[size_t(i)]) {
            TextureDesc d;
            if (Describe(s.textures[size_t(i)], d)) {
                c.texsize[i][0] = float(d.width);
                c.texsize[i][1] = float(d.height);
            }
        }
        const auto& t = s.tss[size_t(i)];
        c.bump[i][0] = Bits(t[kTssBumpEnvMat00]);
        c.bump[i][1] = Bits(t[kTssBumpEnvMat01]);
        c.bump[i][2] = Bits(t[kTssBumpEnvMat10]);
        c.bump[i][3] = Bits(t[kTssBumpEnvMat11]);
        c.bumpl[i][0] = Bits(t[kTssBumpEnvLScale]);
        c.bumpl[i][1] = Bits(t[kTssBumpEnvLOffset]);
    }
}

inline VkStencilOp VkStencilOperation(uint32_t op) {
    switch (op) {
        case 1: return VK_STENCIL_OP_KEEP;
        case 2: return VK_STENCIL_OP_ZERO;
        case 3: return VK_STENCIL_OP_REPLACE;
        case 4: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case 5: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case 6: return VK_STENCIL_OP_INVERT;
        case 7: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case 8: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default: return VK_STENCIL_OP_KEEP;
    }
}

inline VkSamplerAddressMode VkAddress(uint32_t a) {
    switch (a) {
        case 1: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case 2: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case 3: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case 4: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case 5: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        default: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

inline VkFormat HostToVkFormat(Host h) {
    switch (h) {
        case Host::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
        case Host::BC1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case Host::BC2: return VK_FORMAT_BC2_UNORM_BLOCK;
        case Host::BC3: return VK_FORMAT_BC3_UNORM_BLOCK;
        case Host::R32F: return VK_FORMAT_R32_SFLOAT;
        case Host::R16Unorm: return VK_FORMAT_R16_UNORM;
        default: return VK_FORMAT_B8G8R8A8_UNORM;
    }
}

constexpr int kMaxFrames = 3;

class VulkanBackend : public Backend {
public:
    VulkanBackend() = default;
    ~VulkanBackend() override { Cleanup(); }

    bool Init(uint32_t width, uint32_t height) override;
    void Clear(const State& s, uint32_t count, const uint8_t* rects, uint32_t flags, uint32_t color, float z,
               uint32_t stencil) override;
    void Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) override;
    void Copy(const Surface& src, const Surface& dst, uint32_t count, uint32_t src_rects_va,
              uint32_t dst_points_va) override;
    void Present(const Surface& front) override;
    void Sync(uint32_t data_va, uint32_t size) override;
    void SyncLate(uint32_t data_va) override {}
    void Flush() override;
    void SetGamma(const uint8_t* ramp) override;

private:
    bool CreateInstance();
    bool CreateSurface();
    bool PickPhysicalDevice();
    bool CreateDevice();
    bool CreateSwapchain();
    bool CreateRenderPass();
    bool CreateFramebuffers();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    bool InitImGui();
    void Cleanup();
    void RecreateSwapchain();
    void ApplySettings();

    uint32_t FindMemoryType(uint32_t filter, VkMemoryPropertyFlags props);

    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    uint32_t queue_family_ = 0;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapchain_format_ = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchain_extent_{};
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_views_;
    std::vector<VkFramebuffer> framebuffers_;

    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers_;

    VkDescriptorPool imgui_pool_ = VK_NULL_HANDLE;

    std::vector<VkSemaphore> image_available_sem_;
    std::vector<VkSemaphore> render_finished_sem_;
    std::vector<VkFence> in_flight_fences_;
    uint32_t current_frame_ = 0;

    uint32_t guest_w_ = 640;
    uint32_t guest_h_ = 480;
    float scale_ = 1.0f;  // internal resolution multiplier (x1 native, x2, x4...)
    uint8_t ramp_[768] = {};
    bool pass_active_ = false;
    uint32_t image_index_ = 0;
    bool frame_begun_ = false;
    VkFormat depth_format_ = VK_FORMAT_D32_SFLOAT_S8_UINT;

    double fps_ = 60.0;
    uint32_t fps_n_ = 0;
    std::chrono::steady_clock::time_point fps_t_ = std::chrono::steady_clock::now();
    void CountFps() {
        fps_n_++;
        auto now = std::chrono::steady_clock::now();
        double el = std::chrono::duration<double>(now - fps_t_).count();
        if (el >= 0.5) {
            fps_ = fps_n_ / el;
            fps_n_ = 0;
            fps_t_ = now;
        }
    }
    uint64_t pres_count_ = 0;
    VkPipelineCache pipeline_cache_obj_ = VK_NULL_HANDLE;

    void BeginFrame();
    bool EnsureStagingBuffer(size_t size);
    bool EnsureFrontImage(uint32_t w, uint32_t h);
    void TransitionImageLayout(VkCommandBuffer cb, VkImage image,
                               VkImageLayout old_layout, VkImageLayout new_layout,
                               VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT);

    VkBuffer staging_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory_ = VK_NULL_HANDLE;
    void* staging_mapped_ = nullptr;
    size_t staging_size_ = 0;

    VkImage front_image_ = VK_NULL_HANDLE;
    VkDeviceMemory front_image_memory_ = VK_NULL_HANDLE;
    uint32_t front_image_w_ = 0;
    uint32_t front_image_h_ = 0;

    DxcCompiler dxc_;
    std::unordered_map<uint64_t, VulkanTarget> targets_;
    VulkanTarget* EnsureTarget(const Surface& s, bool depth);
    VulkanTarget* bound_rt_ = nullptr;
    VulkanTarget* bound_ds_ = nullptr;
    std::unordered_map<uint64_t, VkShaderModule> shader_cache_;
    VkShaderModule GetOrCreateShader(const std::string& hlsl, const wchar_t* entry, const wchar_t* profile, uint64_t hash_key);
    // SPIR-V kept across runs so DXC does not recompile every shader at each launch
    struct SpirvEntry {
        uint64_t hlsl_hash = 0;
        std::vector<uint32_t> words;
    };
    std::unordered_map<uint64_t, SpirvEntry> spirv_cache_;
    bool spirv_cache_dirty_ = false;
    void LoadSpirvCache();
    void SaveSpirvCache();
    std::vector<uint8_t> SerializeSpirvCache();
    // Shutdown() ends the process with _Exit, so the caches are kept fresh from here, off the frame
    void SaveCachesAsync();
    size_t last_saved_pso_count_ = 0;

    // Dynamic ring buffer
    struct DynamicAlloc {
        VkBuffer buffer = VK_NULL_HANDLE;
        uint32_t offset = 0;
        uint8_t* host_ptr = nullptr;
    };
    bool CreateDynamicBuffer();
    DynamicAlloc AllocDynamic(size_t size, size_t alignment = 256);
    VkBuffer dynamic_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory dynamic_memory_ = VK_NULL_HANDLE;
    void* dynamic_mapped_ = nullptr;
    size_t dynamic_size_ = 64 * 1024 * 1024;
    size_t dynamic_offset_ = 0;
    uint32_t min_uniform_alignment_ = 256;

    // Target render passes
    bool CreateTargetRenderPasses();
    VkRenderPass target_rp_depth_ = VK_NULL_HANDLE;
    VkRenderPass target_rp_color_ = VK_NULL_HANDLE;
    VkRenderPass active_rp_ = VK_NULL_HANDLE;
    VkFramebuffer active_fb_ = VK_NULL_HANDLE;

    // Framebuffer cache for targets
    struct FbKey {
        VkImageView rt_view = VK_NULL_HANDLE;
        VkImageView dt_view = VK_NULL_HANDLE;
        bool operator==(const FbKey& o) const { return rt_view == o.rt_view && dt_view == o.dt_view; }
    };
    struct FbKeyHash {
        size_t operator()(const FbKey& k) const {
            return (uintptr_t)k.rt_view ^ ((uintptr_t)k.dt_view << 1);
        }
    };
    std::unordered_map<FbKey, VkFramebuffer, FbKeyHash> target_fb_cache_;
    VkFramebuffer EnsureFramebuffer(VulkanTarget* rt, VulkanTarget* dt);

    // Dummy texture & sampler
    bool CreateDummyTextureAndSampler();
    VkImage dummy_image_ = VK_NULL_HANDLE;
    VkDeviceMemory dummy_memory_ = VK_NULL_HANDLE;
    VkImageView dummy_view_ = VK_NULL_HANDLE;
    VkSampler dummy_sampler_ = VK_NULL_HANDLE;

    // Sampler cache
    std::unordered_map<uint32_t, VkSampler> sampler_cache_;
    VkSampler GetOrCreateSampler(uint32_t u, uint32_t v, uint32_t min_f, uint32_t mag_f);

    // Texture cache
    struct VulkanTexture {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        uint64_t sig = 0;
        uint64_t palette = 0;
        uint64_t frame = 0;
    };
    std::unordered_map<uint64_t, VulkanTexture> textures_;
    VulkanTexture* GetOrCreateTexture(const State& s, int stage, const TextureDesc& d);

    // Descriptor Layout & Pool
    bool CreateDrawDescriptorLayout();
    VkDescriptorSetLayout draw_ds_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout draw_pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool draw_desc_pool_ = VK_NULL_HANDLE;

    struct DescSetKey {
        VkImageView views[4] = {};
        VkSampler samplers[4] = {};
        bool operator==(const DescSetKey& o) const {
            return std::memcmp(this, &o, sizeof(*this)) == 0;
        }
    };
    struct DescSetHash {
        size_t operator()(const DescSetKey& k) const {
            return Hash(&k, sizeof(k));
        }
    };
    std::unordered_map<DescSetKey, VkDescriptorSet, DescSetHash> ds_cache_;
    VkImageView ds_last_views_[4] = {};
    VkSampler ds_last_samplers_[4] = {};
    VkDescriptorSet ds_last_ds_ = VK_NULL_HANDLE;
    VkDescriptorSet GetOrCreateDescriptorSet(const VkImageView views[4], const VkSampler samplers[4]);

    // Pipelines
    struct PipelineKey {
        VkShaderModule vs = VK_NULL_HANDLE;
        VkShaderModule ps = VK_NULL_HANDLE;
        uint32_t regs = 0;
        uint32_t topo = 0;
        uint32_t strip = 0;
        uint32_t cull = 0;
        uint32_t cull_cw = 0;
        uint32_t blend_en = 0;
        uint32_t src_blend = 0;
        uint32_t dst_blend = 0;
        uint32_t blend_op = 0;
        uint32_t wmask = 0;
        uint32_t depth_en = 0;
        uint32_t depth_write = 0;
        uint32_t depth_func = 0;
        uint32_t stencil_en = 0;
        uint32_t has_depth = 0;
        uint32_t element_formats[16] = {};
        uint32_t element_buffers[16] = {};
        uint32_t element_offsets[16] = {};
        uint32_t buffer_strides[16] = {};
        bool operator==(const PipelineKey& o) const {
            return std::memcmp(this, &o, sizeof(*this)) == 0;
        }
    };
    struct PipelineHash {
        size_t operator()(const PipelineKey& k) const {
            return Hash(&k, sizeof(k));
        }
    };
    std::unordered_map<PipelineKey, VkPipeline, PipelineHash> pipeline_cache_;
    VkPipeline GetOrCreatePipeline(const State& s, const Assembled& a, VkShaderModule vs, VkShaderModule ps, bool has_depth);
};

uint32_t VulkanBackend::FindMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return 0;
}

bool VulkanBackend::EnsureStagingBuffer(size_t size) {
    if (staging_buffer_ && staging_size_ >= size) return true;
    if (staging_mapped_) {
        vkUnmapMemory(device_, staging_memory_);
        staging_mapped_ = nullptr;
    }
    if (staging_buffer_) {
        vkDestroyBuffer(device_, staging_buffer_, nullptr);
        staging_buffer_ = VK_NULL_HANDLE;
    }
    if (staging_memory_) {
        vkFreeMemory(device_, staging_memory_, nullptr);
        staging_memory_ = VK_NULL_HANDLE;
    }

    staging_size_ = (size + 0xFFFF) & ~0xFFFF;
    if (staging_size_ < 16 * 1024 * 1024) staging_size_ = 16 * 1024 * 1024;

    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = staging_size_;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &bci, nullptr, &staging_buffer_) != VK_SUCCESS) return false;

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(device_, staging_buffer_, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device_, &mai, nullptr, &staging_memory_) != VK_SUCCESS) return false;
    vkBindBufferMemory(device_, staging_buffer_, staging_memory_, 0);
    return vkMapMemory(device_, staging_memory_, 0, staging_size_, 0, &staging_mapped_) == VK_SUCCESS;
}

bool VulkanBackend::EnsureFrontImage(uint32_t w, uint32_t h) {
    if (front_image_ && front_image_w_ == w && front_image_h_ == h) return true;
    if (front_image_) {
        vkDestroyImage(device_, front_image_, nullptr);
        front_image_ = VK_NULL_HANDLE;
    }
    if (front_image_memory_) {
        vkFreeMemory(device_, front_image_memory_, nullptr);
        front_image_memory_ = VK_NULL_HANDLE;
    }

    front_image_w_ = w;
    front_image_h_ = h;

    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = VK_FORMAT_B8G8R8A8_UNORM;
    ici.extent = {w, h, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device_, &ici, nullptr, &front_image_) != VK_SUCCESS) return false;

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(device_, front_image_, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device_, &mai, nullptr, &front_image_memory_) != VK_SUCCESS) return false;
    return vkBindImageMemory(device_, front_image_, front_image_memory_, 0) == VK_SUCCESS;
}

void VulkanBackend::TransitionImageLayout(VkCommandBuffer cb, VkImage image,
                                          VkImageLayout old_layout, VkImageLayout new_layout,
                                          VkImageAspectFlags aspect_mask) {
    if (old_layout == new_layout) return;
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        barrier.srcAccessMask = 0;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    }

    if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    } else if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.dstAccessMask = 0;
        dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    } else {
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    }

    vkCmdPipelineBarrier(cb, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

VulkanTarget* VulkanBackend::EnsureTarget(const Surface& s, bool depth) {
    if (!s.data && !s.header) return nullptr;
    uint32_t data = mem::VirtualFromPhysical(s.data);
    uint32_t gw = 640, gh = 480;
    SurfaceSize(s, gw, gh);
    uint64_t key = uint64_t(data) | (depth ? 0x8000000000000000ull : 0);
    auto it = targets_.find(key);
    if (it != targets_.end() && it->second.gw == gw && it->second.gh == gh) {
        return &it->second;
    }

    VulkanTarget t;
    t.depth = depth;
    t.gw = gw;
    t.gh = gh;
    // internal resolution: the host target at the scale multiplier, the viewport maps guest coords onto it
    t.w = std::max(1u, uint32_t(std::lround(gw * scale_)));
    t.h = std::max(1u, uint32_t(std::lround(gh * scale_)));
    t.data = data;
    t.format = depth ? depth_format_ : VK_FORMAT_B8G8R8A8_UNORM;

    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = t.format;
    ici.extent = {t.w, t.h, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = depth ? (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
                      : (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device_, &ici, nullptr, &t.image) != VK_SUCCESS) {
        LOGR("vulkan: EnsureTarget vkCreateImage failed");
        return nullptr;
    }

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(device_, t.image, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device_, &mai, nullptr, &t.memory) != VK_SUCCESS) {
        LOGR("vulkan: EnsureTarget vkAllocateMemory failed");
        return nullptr;
    }
    vkBindImageMemory(device_, t.image, t.memory, 0);

    VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (depth) {
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (t.format == VK_FORMAT_D24_UNORM_S8_UINT || t.format == VK_FORMAT_D32_SFLOAT_S8_UINT || t.format == VK_FORMAT_D16_UNORM_S8_UINT) {
            aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ivci.image = t.image;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = t.format;
    ivci.subresourceRange.aspectMask = aspect_mask;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &ivci, nullptr, &t.view) != VK_SUCCESS) {
        LOGR("vulkan: EnsureTarget vkCreateImageView failed");
        return nullptr;
    }

    t.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    targets_[key] = t;
    LOGR("vulkan: created Target data=0x%08X %ux%u (depth=%d format=%d)", data, gw, gh, depth, t.format);
    return &targets_[key];
}

VkShaderModule VulkanBackend::GetOrCreateShader(const std::string& hlsl, const wchar_t* entry, const wchar_t* profile, uint64_t hash_key) {
    auto it = shader_cache_.find(hash_key);
    if (it != shader_cache_.end()) return it->second;

    uint64_t hlsl_hash = Hash(hlsl.data(), hlsl.size());
    std::vector<uint32_t> spirv;
    auto dit = spirv_cache_.find(hash_key);
    if (dit != spirv_cache_.end() && dit->second.hlsl_hash == hlsl_hash && !dit->second.words.empty()) {
        spirv = dit->second.words;  // keep the cache copy for the next run
    } else {
        spirv = dxc_.Compile(hlsl, entry, profile);
        if (spirv.empty()) {
            LOGR("vulkan: failed to compile shader to SPIR-V");
            return VK_NULL_HANDLE;
        }
        spirv_cache_[hash_key] = SpirvEntry{hlsl_hash, spirv};
        spirv_cache_dirty_ = true;
    }

    VkShaderModuleCreateInfo smci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    smci.codeSize = spirv.size() * sizeof(uint32_t);
    smci.pCode = spirv.data();

    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device_, &smci, nullptr, &mod) != VK_SUCCESS) {
        LOGR("vulkan: vkCreateShaderModule failed");
        return VK_NULL_HANDLE;
    }

    shader_cache_[hash_key] = mod;
    return mod;
}

void VulkanBackend::LoadSpirvCache() {
    std::ifstream in("vk_shader.cache", std::ios::binary);
    if (!in) return;
    uint32_t magic = 0, count = 0;
    in.read(reinterpret_cast<char*>(&magic), 4);
    in.read(reinterpret_cast<char*>(&count), 4);
    if (!in || magic != 0x314B5356u) {  // "VSK1"
        LOGR("vulkan: vk_shader.cache unreadable, recompiling shaders");
        return;
    }
    for (uint32_t i = 0; i < count && in; i++) {
        uint64_t key = 0, hlsl_hash = 0;
        uint32_t words = 0;
        in.read(reinterpret_cast<char*>(&key), 8);
        in.read(reinterpret_cast<char*>(&hlsl_hash), 8);
        in.read(reinterpret_cast<char*>(&words), 4);
        if (!in || words > (64u << 20)) break;
        SpirvEntry e;
        e.hlsl_hash = hlsl_hash;
        e.words.resize(words);
        in.read(reinterpret_cast<char*>(e.words.data()), std::streamsize(words) * 4);
        if (!in) break;
        spirv_cache_[key] = std::move(e);
    }
    LOGR("vulkan: loaded %zu shaders from vk_shader.cache", spirv_cache_.size());
}

std::vector<uint8_t> VulkanBackend::SerializeSpirvCache() {
    std::vector<uint8_t> bytes;
    auto put = [&bytes](const void* p, size_t n) {
        const uint8_t* b = static_cast<const uint8_t*>(p);
        bytes.insert(bytes.end(), b, b + n);
    };
    uint32_t magic = 0x314B5356u, count = uint32_t(spirv_cache_.size());
    put(&magic, 4);
    put(&count, 4);
    for (auto& [key, e] : spirv_cache_) {
        uint32_t words = uint32_t(e.words.size());
        put(&key, 8);
        put(&e.hlsl_hash, 8);
        put(&words, 4);
        put(e.words.data(), size_t(words) * 4);
    }
    return bytes;
}

void VulkanBackend::SaveSpirvCache() {
    if (!spirv_cache_dirty_ || spirv_cache_.empty()) return;
    std::vector<uint8_t> bytes = SerializeSpirvCache();
    std::ofstream out("vk_shader.cache", std::ios::binary | std::ios::trunc);
    if (!out) return;
    out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    if (out) {
        spirv_cache_dirty_ = false;
        LOGR("vulkan: saved %zu shaders to vk_shader.cache", spirv_cache_.size());
    }
}

void VulkanBackend::SaveCachesAsync() {
    // the process leaves through _Exit (no destructor runs), so the caches are written from here every so
    // often; the bytes are snapshotted on this thread, the disk waits on one of its own
    static std::atomic<bool> s_saving{false};
    if (s_saving.load(std::memory_order_relaxed)) return;
    bool want_pso = pipeline_cache_obj_ && vkGetPipelineCacheData && pipeline_cache_.size() > last_saved_pso_count_;
    bool want_spirv = spirv_cache_dirty_ && !spirv_cache_.empty();
    if (!want_pso && !want_spirv) return;
    if (s_saving.exchange(true)) return;

    auto pso = std::make_shared<std::vector<uint8_t>>();
    if (want_pso) {
        size_t sz = 0;
        if (vkGetPipelineCacheData(device_, pipeline_cache_obj_, &sz, nullptr) == VK_SUCCESS && sz > 0) {
            pso->resize(sz);
            if (vkGetPipelineCacheData(device_, pipeline_cache_obj_, &sz, pso->data()) == VK_SUCCESS)
                last_saved_pso_count_ = pipeline_cache_.size();
            else
                pso->clear();
        }
    }
    auto spirv = want_spirv ? std::make_shared<std::vector<uint8_t>>(SerializeSpirvCache()) : nullptr;
    if (spirv && !spirv->empty()) spirv_cache_dirty_ = false;

    std::thread([pso, spirv] {  // s_saving is a local static: reachable without capture
        // temp file + rename: the process can die (_Exit) mid-write without leaving a broken cache
        auto store = [](const char* path, const std::vector<uint8_t>& bytes) {
            if (bytes.empty()) return;
            std::string tmp = std::string(path) + ".tmp";
            {
                std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
                if (!out) return;
                out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                if (!out) return;
            }
            std::error_code ec;
            std::filesystem::rename(tmp, path, ec);
        };
        store("vk_pipeline.cache", *pso);
        if (spirv) store("vk_shader.cache", *spirv);
        s_saving.store(false, std::memory_order_relaxed);
    }).detach();
}

bool VulkanBackend::CreateInstance() {
    VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = "Bloody Roar Extreme Recomp";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "XboxXBE-Recomp Vulkan";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app_info;
    ci.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
    ci.ppEnabledExtensionNames = extensions;

    VkResult res = vkCreateInstance(&ci, nullptr, &instance_);
    if (res != VK_SUCCESS) {
        LOGE("vulkan: vkCreateInstance failed (%d)", res);
        return false;
    }
    LoadVulkanInstance(instance_);
    return true;
}

bool VulkanBackend::CreateSurface() {
    HWND hwnd = static_cast<HWND>(app::Window());
    if (!hwnd) {
        LOGR("vulkan: app::Window() returned null hwnd");
        return false;
    }
    VkWin32SurfaceCreateInfoKHR sci{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    sci.hinstance = GetModuleHandle(nullptr);
    sci.hwnd = hwnd;
    VkResult res = vkCreateWin32SurfaceKHR(instance_, &sci, nullptr, &surface_);
    if (res != VK_SUCCESS) {
        LOGR("vulkan: vkCreateWin32SurfaceKHR failed (%d)", res);
        return false;
    }
    LOGR("vulkan: surface created for HWND %p", hwnd);
    return true;
}

bool VulkanBackend::PickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (!count) {
        LOGR("vulkan: no physical devices found");
        return false;
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    LOGR("vulkan: found %u physical device(s)", count);

    for (auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        LOGR("vulkan: candidate device '%s' (type %u)", props.deviceName, props.deviceType);
        uint32_t qf_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qf_count, nullptr);
        std::vector<VkQueueFamilyProperties> qf(qf_count);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qf_count, qf.data());

        for (uint32_t i = 0; i < qf_count; i++) {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface_, &present_support);
            if ((qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present_support) {
                physical_device_ = dev;
                queue_family_ = i;
                LOGR("vulkan: selected physical device '%s'", props.deviceName);
                min_uniform_alignment_ = std::max<uint32_t>(256, static_cast<uint32_t>(props.limits.minUniformBufferOffsetAlignment));

                VkFormat depth_candidates[] = {
                    VK_FORMAT_D24_UNORM_S8_UINT,
                    VK_FORMAT_D32_SFLOAT_S8_UINT,
                    VK_FORMAT_D16_UNORM_S8_UINT
                };
                depth_format_ = VK_FORMAT_D32_SFLOAT_S8_UINT;
                for (auto f : depth_candidates) {
                    VkFormatProperties fprops{};
                    vkGetPhysicalDeviceFormatProperties(dev, f, &fprops);
                    if ((fprops.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                        depth_format_ = f;
                        break;
                    }
                }
                LOGR("vulkan: selected depth format %d", depth_format_);
                return true;
            }
        }
    }
    LOGR("vulkan: no device with graphics and surface support found");
    return false;
}

bool VulkanBackend::CreateDevice() {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = queue_family_;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* dev_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = static_cast<uint32_t>(std::size(dev_extensions));
    dci.ppEnabledExtensionNames = dev_extensions;

    VkResult res = vkCreateDevice(physical_device_, &dci, nullptr, &device_);
    if (res != VK_SUCCESS) {
        LOGR("vulkan: vkCreateDevice failed (%d)", res);
        return false;
    }
    LoadVulkanDevice(device_);
    vkGetDeviceQueue(device_, queue_family_, 0, &queue_);
    LOGR("vulkan: logical device and queue created");
    return true;
}

bool VulkanBackend::CreateSwapchain() {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &caps);

    swapchain_extent_ = caps.currentExtent;
    if (swapchain_extent_.width == UINT32_MAX || swapchain_extent_.width == 0) {
        swapchain_extent_.width = guest_w_ ? guest_w_ : 1280;
    }
    if (swapchain_extent_.height == UINT32_MAX || swapchain_extent_.height == 0) {
        swapchain_extent_.height = guest_h_ ? guest_h_ : 720;
    }
    if (caps.minImageExtent.width > 0 && caps.maxImageExtent.width > 0) {
        swapchain_extent_.width = std::clamp(swapchain_extent_.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        swapchain_extent_.height = std::clamp(swapchain_extent_.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    uint32_t fmt_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &fmt_count, nullptr);
    if (fmt_count > 0) {
        std::vector<VkSurfaceFormatKHR> formats(fmt_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &fmt_count, formats.data());
        swapchain_format_ = formats[0].format;
        for (const auto& f : formats) {
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) {
                swapchain_format_ = f.format;
                break;
            }
        }
    }

    uint32_t img_count = std::max(3u, caps.minImageCount + 1);
    if (caps.maxImageCount > 0 && img_count > caps.maxImageCount) {
        img_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface = surface_;
    sci.minImageCount = img_count;
    sci.imageFormat = swapchain_format_;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = swapchain_extent_;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    if (!app::Get().vsync) {
        uint32_t pm_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &pm_count, nullptr);
        std::vector<VkPresentModeKHR> pms(pm_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &pm_count, pms.data());
        for (auto m : pms) {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
                present_mode = m;
                break;
            }
            if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                present_mode = m;
            }
        }
    }
    sci.presentMode = present_mode;
    sci.clipped = VK_TRUE;

    VkResult res = vkCreateSwapchainKHR(device_, &sci, nullptr, &swapchain_);
    if (res != VK_SUCCESS) {
        LOGR("vulkan: vkCreateSwapchainKHR failed (%d)", res);
        return false;
    }
    LOGR("vulkan: swapchain created: %ux%u (format %d, images %u)", swapchain_extent_.width, swapchain_extent_.height, swapchain_format_, img_count);

    uint32_t actual_imgs = 0;
    vkGetSwapchainImagesKHR(device_, swapchain_, &actual_imgs, nullptr);
    swapchain_images_.resize(actual_imgs);
    vkGetSwapchainImagesKHR(device_, swapchain_, &actual_imgs, swapchain_images_.data());

    swapchain_views_.resize(actual_imgs);
    for (uint32_t i = 0; i < actual_imgs; i++) {
        VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ivci.image = swapchain_images_[i];
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = swapchain_format_;
        ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;
        vkCreateImageView(device_, &ivci, nullptr, &swapchain_views_[i]);
    }
    return true;
}

bool VulkanBackend::CreateRenderPass() {
    VkAttachmentDescription color_att{};
    color_att.format = swapchain_format_;
    color_att.samples = VK_SAMPLE_COUNT_1_BIT;
    color_att.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_att.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpci.attachmentCount = 1;
    rpci.pAttachments = &color_att;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;

    return vkCreateRenderPass(device_, &rpci, nullptr, &render_pass_) == VK_SUCCESS;
}

bool VulkanBackend::CreateFramebuffers() {
    framebuffers_.resize(swapchain_views_.size());
    for (size_t i = 0; i < swapchain_views_.size(); i++) {
        VkImageView atts[] = {swapchain_views_[i]};
        VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fci.renderPass = render_pass_;
        fci.attachmentCount = 1;
        fci.pAttachments = atts;
        fci.width = swapchain_extent_.width;
        fci.height = swapchain_extent_.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(device_, &fci, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            return false;
        }
    }
    return true;
}

bool VulkanBackend::CreateCommandBuffers() {
    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.queueFamilyIndex = queue_family_;
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device_, &pci, nullptr, &command_pool_) != VK_SUCCESS) return false;

    command_buffers_.resize(kMaxFrames);
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = command_pool_;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = kMaxFrames;
    return vkAllocateCommandBuffers(device_, &ai, command_buffers_.data()) == VK_SUCCESS;
}

bool VulkanBackend::CreateSyncObjects() {
    image_available_sem_.resize(kMaxFrames);
    render_finished_sem_.resize(kMaxFrames);
    in_flight_fences_.resize(kMaxFrames);

    VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < kMaxFrames; i++) {
        vkCreateSemaphore(device_, &sci, nullptr, &image_available_sem_[i]);
        vkCreateSemaphore(device_, &sci, nullptr, &render_finished_sem_[i]);
        vkCreateFence(device_, &fci, nullptr, &in_flight_fences_[i]);
    }
    return true;
}

bool VulkanBackend::InitImGui() {
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };
    VkDescriptorPoolCreateInfo dpci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dpci.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    dpci.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
    dpci.pPoolSizes = pool_sizes;
    vkCreateDescriptorPool(device_, &dpci, nullptr, &imgui_pool_);

    struct LoaderContext {
        VkInstance inst;
        VkDevice dev;
    } ctx{instance_, device_};

    ImGui_ImplVulkan_LoadFunctions([](const char* fn, void* user_data) -> PFN_vkVoidFunction {
        auto* c = static_cast<LoaderContext*>(user_data);
        auto p = vkGetDeviceProcAddr(c->dev, fn);
        if (!p) p = vkGetInstanceProcAddr(c->inst, fn);
        return p;
    }, &ctx);

    ImGui_ImplVulkan_InitInfo ii{};
    ii.Instance = instance_;
    ii.PhysicalDevice = physical_device_;
    ii.Device = device_;
    ii.QueueFamily = queue_family_;
    ii.Queue = queue_;
    ii.DescriptorPool = imgui_pool_;
    ii.Subpass = 0;
    ii.MinImageCount = kMaxFrames;
    ii.ImageCount = static_cast<uint32_t>(swapchain_images_.size());
    ii.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ii.RenderPass = render_pass_;
    ImGui_ImplVulkan_Init(&ii);

    return true;
}

bool VulkanBackend::Init(uint32_t width, uint32_t height) {
    guest_w_ = width;
    guest_h_ = height;
    scale_ = app::RenderScale(guest_h_);
    LOGR("vulkan: initializing native Vulkan backend for %ux%u", width, height);

    if (!LoadVulkanGlobal()) { LOGR("vulkan: LoadVulkanGlobal failed"); return false; }
    LOGR("vulkan: LoadVulkanGlobal ok");

    if (!CreateInstance()) { LOGR("vulkan: CreateInstance failed"); return false; }
    LOGR("vulkan: CreateInstance ok");

    if (!CreateSurface()) { LOGR("vulkan: CreateSurface failed"); return false; }
    LOGR("vulkan: CreateSurface ok");

    if (!PickPhysicalDevice()) { LOGR("vulkan: PickPhysicalDevice failed"); return false; }
    LOGR("vulkan: PickPhysicalDevice ok");

    if (!CreateDevice()) { LOGR("vulkan: CreateDevice failed"); return false; }
    LOGR("vulkan: CreateDevice ok");

    if (!CreateSwapchain()) { LOGR("vulkan: CreateSwapchain failed"); return false; }
    LOGR("vulkan: CreateSwapchain ok");

    if (!CreateRenderPass()) { LOGR("vulkan: CreateRenderPass failed"); return false; }
    LOGR("vulkan: CreateRenderPass ok");

    if (!CreateFramebuffers()) { LOGR("vulkan: CreateFramebuffers failed"); return false; }
    LOGR("vulkan: CreateFramebuffers ok");

    if (!CreateCommandBuffers()) { LOGR("vulkan: CreateCommandBuffers failed"); return false; }
    LOGR("vulkan: CreateCommandBuffers ok");

    if (!CreateSyncObjects()) { LOGR("vulkan: CreateSyncObjects failed"); return false; }
    LOGR("vulkan: CreateSyncObjects ok");

    if (!CreateDynamicBuffer()) { LOGR("vulkan: CreateDynamicBuffer failed"); return false; }
    LOGR("vulkan: CreateDynamicBuffer ok");

    if (!CreateTargetRenderPasses()) { LOGR("vulkan: CreateTargetRenderPasses failed"); return false; }
    LOGR("vulkan: CreateTargetRenderPasses ok");

    if (!CreateDrawDescriptorLayout()) { LOGR("vulkan: CreateDrawDescriptorLayout failed"); return false; }
    LOGR("vulkan: CreateDrawDescriptorLayout ok");

    if (!CreateDummyTextureAndSampler()) { LOGR("vulkan: CreateDummyTextureAndSampler failed"); return false; }
    LOGR("vulkan: CreateDummyTextureAndSampler ok");

    std::vector<uint8_t> cache_data;
    std::ifstream cache_in("vk_pipeline.cache", std::ios::binary | std::ios::ate);
    if (cache_in) {
        std::streamsize sz = cache_in.tellg();
        cache_in.seekg(0, std::ios::beg);
        cache_data.resize(sz);
        cache_in.read(reinterpret_cast<char*>(cache_data.data()), sz);
        cache_in.close();
        LOGR("vulkan: loaded %zu bytes from vk_pipeline.cache", cache_data.size());
    }

    VkPipelineCacheCreateInfo pcci{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    pcci.initialDataSize = cache_data.size();
    pcci.pInitialData = cache_data.empty() ? nullptr : cache_data.data();
    if (vkCreatePipelineCache(device_, &pcci, nullptr, &pipeline_cache_obj_) != VK_SUCCESS) {
        LOGR("vulkan: vkCreatePipelineCache failed");
    } else {
        LOGR("vulkan: pipeline cache created");
    }

    dxc_.Init();
    LoadSpirvCache();
    InitImGui();
    LOGR("vulkan: native Vulkan backend initialized successfully");
    return true;
}

void VulkanBackend::BeginFrame() {
    if (frame_begun_) return;
    vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);

    VkCommandBuffer cb = command_buffers_[current_frame_];
    vkResetCommandBuffer(cb, 0);

    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cb, &bi);

    dynamic_offset_ = current_frame_ * (dynamic_size_ / kMaxFrames);
    active_rp_ = VK_NULL_HANDLE;
    active_fb_ = VK_NULL_HANDLE;
    frame_begun_ = true;
}

void VulkanBackend::Clear(const State& s, uint32_t count, const uint8_t*, uint32_t flags, uint32_t color, float z,
                          uint32_t stencil) {
    static uint64_t clear_count = 0;
    if (++clear_count <= 10) {
        LOGR("vulkan: Clear #%llu color=0x%08X flags=0x%X z=%f stencil=%u",
             (unsigned long long)clear_count, color, flags, z, stencil);
    }

    BeginFrame();
    VkCommandBuffer cb = command_buffers_[current_frame_];
    if (active_rp_ != VK_NULL_HANDLE) {
        vkCmdEndRenderPass(cb);
        active_rp_ = VK_NULL_HANDLE;
        active_fb_ = VK_NULL_HANDLE;
    }

    if (flags & kClearTarget) {
        VulkanTarget* rt = EnsureTarget(s.rt, false);
        if (rt) {
            TransitionImageLayout(cb, rt->image, rt->layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            rt->layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            VkClearColorValue cv{};
            cv.float32[0] = ((color >> 16) & 0xFF) / 255.0f;
            cv.float32[1] = ((color >> 8) & 0xFF) / 255.0f;
            cv.float32[2] = (color & 0xFF) / 255.0f;
            cv.float32[3] = ((color >> 24) & 0xFF) / 255.0f;

            VkImageSubresourceRange srr{};
            srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            srr.levelCount = 1;
            srr.layerCount = 1;
            vkCmdClearColorImage(cb, rt->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cv, 1, &srr);
            rt->drawn = true;
            rt->epoch = DirtyEpoch(rt->data);
        }
    }

    if (flags & (kClearZ | kClearStencil)) {
        VulkanTarget* dt = EnsureTarget(s.zs, true);
        if (dt) {
            VkImageAspectFlags aspect = 0;
            if (flags & kClearZ) aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
            if ((flags & kClearStencil) && (dt->format == VK_FORMAT_D24_UNORM_S8_UINT || dt->format == VK_FORMAT_D32_SFLOAT_S8_UINT || dt->format == VK_FORMAT_D16_UNORM_S8_UINT)) {
                aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            TransitionImageLayout(cb, dt->image, dt->layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aspect);
            dt->layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            VkClearDepthStencilValue dsv{};
            dsv.depth = z;
            dsv.stencil = stencil;

            VkImageSubresourceRange srr{};
            srr.aspectMask = aspect;
            srr.levelCount = 1;
            srr.layerCount = 1;
            vkCmdClearDepthStencilImage(cb, dt->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &dsv, 1, &srr);
            dt->drawn = true;
        }
    }
}

bool VulkanBackend::CreateDynamicBuffer() {
    dynamic_size_ = 64 * 1024 * 1024;
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = dynamic_size_;
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &bci, nullptr, &dynamic_buffer_) != VK_SUCCESS) {
        LOGR("vulkan: CreateDynamicBuffer vkCreateBuffer failed");
        return false;
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(device_, dynamic_buffer_, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device_, &mai, nullptr, &dynamic_memory_) != VK_SUCCESS) {
        LOGR("vulkan: CreateDynamicBuffer vkAllocateMemory failed");
        return false;
    }
    vkBindBufferMemory(device_, dynamic_buffer_, dynamic_memory_, 0);
    if (vkMapMemory(device_, dynamic_memory_, 0, dynamic_size_, 0, &dynamic_mapped_) != VK_SUCCESS) {
        LOGR("vulkan: CreateDynamicBuffer vkMapMemory failed");
        return false;
    }
    LOGR("vulkan: dynamic buffer created (64MB host-visible/coherent)");
    return true;
}

VulkanBackend::DynamicAlloc VulkanBackend::AllocDynamic(size_t size, size_t alignment) {
    size_t frame_chunk = dynamic_size_ / kMaxFrames;
    size_t frame_start = current_frame_ * frame_chunk;
    size_t frame_end = frame_start + frame_chunk;
    dynamic_offset_ = (dynamic_offset_ + alignment - 1) & ~(alignment - 1);
    if (dynamic_offset_ < frame_start || dynamic_offset_ + size > frame_end) {
        dynamic_offset_ = frame_start;
    }
    DynamicAlloc a;
    a.buffer = dynamic_buffer_;
    a.offset = static_cast<uint32_t>(dynamic_offset_);
    a.host_ptr = static_cast<uint8_t*>(dynamic_mapped_) + dynamic_offset_;
    dynamic_offset_ += size;
    return a;
}

bool VulkanBackend::CreateTargetRenderPasses() {
    // 1. Color + Depth
    {
        VkAttachmentDescription atts[2]{};
        // Color
        atts[0].format = VK_FORMAT_B8G8R8A8_UNORM;
        atts[0].samples = VK_SAMPLE_COUNT_1_BIT;
        atts[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        atts[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        atts[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        atts[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        atts[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        atts[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Depth
        atts[1].format = depth_format_;
        atts[1].samples = VK_SAMPLE_COUNT_1_BIT;
        atts[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        atts[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        atts[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        atts[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        atts[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        atts[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depth_ref{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_ref;
        subpass.pDepthStencilAttachment = &depth_ref;

        VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpci.attachmentCount = 2;
        rpci.pAttachments = atts;
        rpci.subpassCount = 1;
        rpci.pSubpasses = &subpass;

        if (vkCreateRenderPass(device_, &rpci, nullptr, &target_rp_depth_) != VK_SUCCESS) {
            LOGR("vulkan: failed to create target_rp_depth_");
            return false;
        }
    }

    // 2. Color only
    {
        VkAttachmentDescription att{};
        att.format = VK_FORMAT_B8G8R8A8_UNORM;
        att.samples = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_ref;

        VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpci.attachmentCount = 1;
        rpci.pAttachments = &att;
        rpci.subpassCount = 1;
        rpci.pSubpasses = &subpass;

        if (vkCreateRenderPass(device_, &rpci, nullptr, &target_rp_color_) != VK_SUCCESS) {
            LOGR("vulkan: failed to create target_rp_color_");
            return false;
        }
    }

    LOGR("vulkan: target render passes created (depth + color)");
    return true;
}

VkFramebuffer VulkanBackend::EnsureFramebuffer(VulkanTarget* rt, VulkanTarget* dt) {
    if (!rt) return VK_NULL_HANDLE;
    FbKey key{rt->view, dt ? dt->view : VK_NULL_HANDLE};
    auto it = target_fb_cache_.find(key);
    if (it != target_fb_cache_.end()) return it->second;

    VkImageView atts[2] = {rt->view, dt ? dt->view : VK_NULL_HANDLE};
    VkFramebufferCreateInfo fbci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbci.renderPass = dt ? target_rp_depth_ : target_rp_color_;
    fbci.attachmentCount = dt ? 2 : 1;
    fbci.pAttachments = atts;
    fbci.width = rt->w;
    fbci.height = rt->h;
    fbci.layers = 1;

    VkFramebuffer fb = VK_NULL_HANDLE;
    if (vkCreateFramebuffer(device_, &fbci, nullptr, &fb) != VK_SUCCESS) {
        LOGR("vulkan: EnsureFramebuffer vkCreateFramebuffer failed");
        return VK_NULL_HANDLE;
    }
    target_fb_cache_[key] = fb;
    return fb;
}

bool VulkanBackend::CreateDummyTextureAndSampler() {
    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = VK_FORMAT_R8G8B8A8_UNORM;
    ici.extent = {1, 1, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device_, &ici, nullptr, &dummy_image_) != VK_SUCCESS) return false;

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(device_, dummy_image_, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device_, &mai, nullptr, &dummy_memory_) != VK_SUCCESS) return false;
    vkBindImageMemory(device_, dummy_image_, dummy_memory_, 0);

    VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ivci.image = dummy_image_;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R8G8B8A8_UNORM;
    ivci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (vkCreateImageView(device_, &ivci, nullptr, &dummy_view_) != VK_SUCCESS) return false;

    VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sci.magFilter = VK_FILTER_LINEAR;
    sci.minFilter = VK_FILTER_LINEAR;
    sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device_, &sci, nullptr, &dummy_sampler_) != VK_SUCCESS) return false;

    uint32_t blank = 0x00000000;
    if (EnsureStagingBuffer(4)) {
        std::memcpy(staging_mapped_, &blank, 4);

        VkCommandBufferAllocateInfo cbai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cbai.commandPool = command_pool_;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;
        VkCommandBuffer cb = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(device_, &cbai, &cb);

        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &bi);

        TransitionImageLayout(cb, dummy_image_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {1, 1, 1};
        vkCmdCopyBufferToImage(cb, staging_buffer_, dummy_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        TransitionImageLayout(cb, dummy_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkEndCommandBuffer(cb);

        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cb;
        vkQueueSubmit(queue_, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue_);
        vkFreeCommandBuffers(device_, command_pool_, 1, &cb);
    }
    LOGR("vulkan: dummy 1x1 white texture and sampler created");
    return true;
}

VkSampler VulkanBackend::GetOrCreateSampler(uint32_t u, uint32_t v, uint32_t min_f, uint32_t mag_f) {
    uint32_t key = (u & 7) | ((v & 7) << 3) | ((min_f >= 2 ? 1 : 0) << 6) | ((mag_f >= 2 ? 1 : 0) << 7);
    static uint32_t s_last_key = 0xFFFFFFFF;
    static VkSampler s_last_sampler = VK_NULL_HANDLE;
    if (key == s_last_key && s_last_sampler != VK_NULL_HANDLE) return s_last_sampler;

    auto it = sampler_cache_.find(key);
    if (it != sampler_cache_.end()) {
        s_last_key = key;
        s_last_sampler = it->second;
        return it->second;
    }

    VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sci.magFilter = (mag_f >= 2) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sci.minFilter = (min_f >= 2) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sci.addressModeU = VkAddress(u);
    sci.addressModeV = VkAddress(v);
    sci.addressModeW = sci.addressModeU;
    sci.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler smp = VK_NULL_HANDLE;
    if (vkCreateSampler(device_, &sci, nullptr, &smp) != VK_SUCCESS) {
        return dummy_sampler_;
    }
    sampler_cache_[key] = smp;
    s_last_key = key;
    s_last_sampler = smp;
    return smp;
}

struct StagePalette {
    uint32_t va = 0;
    uint32_t colours[256] = {};
    uint64_t hash = 0;
};

inline void ReadStagePalette(const State& s, int stage, const TextureDesc& d, StagePalette& palette) {
    constexpr uint32_t kFmtP8 = 0x0B;
    constexpr uint32_t kPaletteCommon = 0;
    constexpr uint32_t kPaletteData = 4;
    constexpr uint32_t kPaletteSizeShift = 30;
    if (d.format != kFmtP8 || !s.palettes[size_t(stage)]) return;
    uint32_t header = s.palettes[size_t(stage)];
    palette.va = mem::VirtualFromPhysical(kernel::R32(header + kPaletteData));
    uint32_t size_code = kernel::R32(header + kPaletteCommon) >> kPaletteSizeShift;
    size_t bytes = (std::size(palette.colours) >> size_code) * sizeof(uint32_t);
    std::memcpy(palette.colours, kernel::HostPtr(palette.va), bytes);
    palette.hash = Hash(palette.colours, sizeof(palette.colours));
}

inline uint64_t MemSig(uint32_t data, uint32_t total) {
    constexpr uint32_t kSigWholeMax = 65536;
    constexpr uint32_t kSigSamples = 64;
    constexpr uint32_t kSigSampleBytes = 64;
    const uint8_t* p = static_cast<const uint8_t*>(kernel::HostPtr(data));
    uint64_t sig = Hash(&total, sizeof(total));
    if (total <= kSigWholeMax) return Hash(p, total, sig);
    uint32_t step = (total - kSigSampleBytes) / (kSigSamples - 1);
    for (uint32_t i = 0; i < kSigSamples; i++) sig = Hash(p + i * step, kSigSampleBytes, sig);
    return sig;
}

VulkanBackend::VulkanTexture* VulkanBackend::GetOrCreateTexture(const State& s, int stage, const TextureDesc& d) {
    if (!d.data) return nullptr;
    const FormatInfo& fi = Format(d.format);
    StagePalette palette;
    ReadStagePalette(s, stage, d, palette);
    const uint64_t key = uint64_t(palette.va) << 32 | d.data;

    auto it = textures_.find(key);
    bool same = (it != textures_.end() && it->second.width == d.width && it->second.height == d.height && it->second.image != VK_NULL_HANDLE);

    if (same && it->second.frame == pres_count_ && it->second.palette == palette.hash) {
        return &it->second;
    }

    uint32_t total = TotalSize(d);
    uint64_t sig = MemSig(d.data, total) ^ DirtyEpoch(d.data) ^ palette.hash;

    if (same && it->second.sig == sig && it->second.palette == palette.hash) {
        it->second.frame = pres_count_;
        return &it->second;
    }

    std::vector<uint8_t> px;
    uint32_t pitch = 0;
    DecodeLevel(d, 0, 0, static_cast<const uint8_t*>(kernel::HostPtr(d.data)), palette.va ? palette.colours : nullptr, px, pitch);
    if (px.empty()) return nullptr;

    VkFormat vk_fmt = HostToVkFormat(fi.host);

    if (!same) {
        if (it != textures_.end()) {
            if (it->second.view) vkDestroyImageView(device_, it->second.view, nullptr);
            if (it->second.image) vkDestroyImage(device_, it->second.image, nullptr);
            if (it->second.memory) vkFreeMemory(device_, it->second.memory, nullptr);
            textures_.erase(it);
        }

        VulkanTexture tex;
        tex.width = d.width;
        tex.height = d.height;
        tex.format = vk_fmt;

        VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ici.imageType = VK_IMAGE_TYPE_2D;
        ici.format = vk_fmt;
        ici.extent = {d.width, d.height, 1};
        ici.mipLevels = 1;
        ici.arrayLayers = 1;
        ici.samples = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling = VK_IMAGE_TILING_OPTIMAL;
        ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device_, &ici, nullptr, &tex.image) != VK_SUCCESS) return nullptr;

        VkMemoryRequirements req{};
        vkGetImageMemoryRequirements(device_, tex.image, &req);
        VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        mai.allocationSize = req.size;
        mai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(device_, &mai, nullptr, &tex.memory) != VK_SUCCESS) {
            vkDestroyImage(device_, tex.image, nullptr);
            return nullptr;
        }
        vkBindImageMemory(device_, tex.image, tex.memory, 0);

        VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ivci.image = tex.image;
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = vk_fmt;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device_, &ivci, nullptr, &tex.view) != VK_SUCCESS) {
            vkFreeMemory(device_, tex.memory, nullptr);
            vkDestroyImage(device_, tex.image, nullptr);
            return nullptr;
        }

        tex.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        textures_[key] = tex;
    }

    VulkanTexture& tex = textures_[key];
    tex.sig = sig;
    tex.palette = palette.hash;
    tex.frame = pres_count_;

    DynamicAlloc a = AllocDynamic(px.size(), 256);
    std::memcpy(a.host_ptr, px.data(), px.size());

    VkCommandBuffer cb = command_buffers_[current_frame_];
    if (active_rp_ != VK_NULL_HANDLE) {
        vkCmdEndRenderPass(cb);
        active_rp_ = VK_NULL_HANDLE;
        active_fb_ = VK_NULL_HANDLE;
    }
    TransitionImageLayout(cb, tex.image, tex.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region{};
    region.bufferOffset = a.offset;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {d.width, d.height, 1};
    vkCmdCopyBufferToImage(cb, a.buffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    TransitionImageLayout(cb, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    tex.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return &tex;
}

bool VulkanBackend::CreateDrawDescriptorLayout() {
    VkDescriptorSetLayoutBinding b[10]{};
    // b0: VC (Uniform dynamic)
    b[0].binding = 0;
    b[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    b[0].descriptorCount = 1;
    b[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // b1: DC (Uniform dynamic)
    b[1].binding = 1;
    b[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    b[1].descriptorCount = 1;
    b[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // t0..t3: Sampled images (bindings 10..13)
    for (int i = 0; i < 4; i++) {
        b[2 + i].binding = 10 + i;
        b[2 + i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        b[2 + i].descriptorCount = 1;
        b[2 + i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    // s0..s3: Samplers (bindings 20..23)
    for (int i = 0; i < 4; i++) {
        b[6 + i].binding = 20 + i;
        b[6 + i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        b[6 + i].descriptorCount = 1;
        b[6 + i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo dslci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dslci.bindingCount = 10;
    dslci.pBindings = b;
    if (vkCreateDescriptorSetLayout(device_, &dslci, nullptr, &draw_ds_layout_) != VK_SUCCESS) {
        LOGR("vulkan: failed to create draw_ds_layout_");
        return false;
    }

    VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &draw_ds_layout_;
    if (vkCreatePipelineLayout(device_, &plci, nullptr, &draw_pipeline_layout_) != VK_SUCCESS) {
        LOGR("vulkan: failed to create draw_pipeline_layout_");
        return false;
    }

    // Descriptor pool size balanced for integrated and dedicated GPUs (8192 sets)
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 8192 * 2},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 8192 * 4},
        {VK_DESCRIPTOR_TYPE_SAMPLER, 8192 * 4}
    };
    VkDescriptorPoolCreateInfo dpci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpci.maxSets = 8192;
    dpci.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    dpci.pPoolSizes = pool_sizes;
    if (vkCreateDescriptorPool(device_, &dpci, nullptr, &draw_desc_pool_) != VK_SUCCESS) {
        LOGR("vulkan: failed to create draw_desc_pool_");
        return false;
    }

    LOGR("vulkan: draw descriptor layout and pool created");
    return true;
}

VkDescriptorSet VulkanBackend::GetOrCreateDescriptorSet(const VkImageView views[4], const VkSampler samplers[4]) {
    if (ds_last_ds_ != VK_NULL_HANDLE &&
        std::memcmp(views, ds_last_views_, sizeof(ds_last_views_)) == 0 &&
        std::memcmp(samplers, ds_last_samplers_, sizeof(ds_last_samplers_)) == 0) {
        return ds_last_ds_;
    }

    DescSetKey key{};
    for (int i = 0; i < 4; i++) {
        key.views[i] = views[i];
        key.samplers[i] = samplers[i];
    }
    auto it = ds_cache_.find(key);
    if (it != ds_cache_.end()) {
        std::memcpy(ds_last_views_, views, sizeof(ds_last_views_));
        std::memcpy(ds_last_samplers_, samplers, sizeof(ds_last_samplers_));
        ds_last_ds_ = it->second;
        return it->second;
    }

    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = draw_desc_pool_;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &draw_ds_layout_;

    VkDescriptorSet ds = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(device_, &ai, &ds) != VK_SUCCESS) {
        // Pool exhausted: wait for in-flight GPU work, flush cache, and reset pool to recover gracefully on any hardware
        if (device_ && draw_desc_pool_ && vkResetDescriptorPool) {
            vkDeviceWaitIdle(device_);
            ds_cache_.clear();
            std::memset(ds_last_views_, 0, sizeof(ds_last_views_));
            std::memset(ds_last_samplers_, 0, sizeof(ds_last_samplers_));
            ds_last_ds_ = VK_NULL_HANDLE;
            vkResetDescriptorPool(device_, draw_desc_pool_, 0);
            vkAllocateDescriptorSets(device_, &ai, &ds);
        }
        if (ds == VK_NULL_HANDLE) {
            LOGR("vulkan: vkAllocateDescriptorSets failed");
            return VK_NULL_HANDLE;
        }
    }

    VkDescriptorBufferInfo b0_info{};
    b0_info.buffer = dynamic_buffer_;
    b0_info.offset = 0;
    b0_info.range = 192 * 16;

    VkDescriptorBufferInfo b1_info{};
    b1_info.buffer = dynamic_buffer_;
    b1_info.offset = 0;
    b1_info.range = sizeof(DrawConstants);

    VkDescriptorImageInfo img_info[4]{};
    VkDescriptorImageInfo smp_info[4]{};
    VkWriteDescriptorSet writes[10]{};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = ds;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[0].pBufferInfo = &b0_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = ds;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[1].pBufferInfo = &b1_info;

    for (int i = 0; i < 4; i++) {
        img_info[i].imageView = views[i];
        img_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        writes[2 + i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2 + i].dstSet = ds;
        writes[2 + i].dstBinding = 10 + i;
        writes[2 + i].descriptorCount = 1;
        writes[2 + i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[2 + i].pImageInfo = &img_info[i];

        smp_info[i].sampler = samplers[i];
        writes[6 + i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[6 + i].dstSet = ds;
        writes[6 + i].dstBinding = 20 + i;
        writes[6 + i].descriptorCount = 1;
        writes[6 + i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        writes[6 + i].pImageInfo = &smp_info[i];
    }

    vkUpdateDescriptorSets(device_, 10, writes, 0, nullptr);
    ds_cache_[key] = ds;
    std::memcpy(ds_last_views_, views, sizeof(ds_last_views_));
    std::memcpy(ds_last_samplers_, samplers, sizeof(ds_last_samplers_));
    ds_last_ds_ = ds;
    return ds;
}

VkPipeline VulkanBackend::GetOrCreatePipeline(const State& s, const Assembled& a, VkShaderModule vs, VkShaderModule ps, bool has_depth) {
    PipelineKey k{};
    k.vs = vs;
    k.ps = ps;
    k.regs = a.regs;
    k.topo = static_cast<uint32_t>(a.topology);
    k.strip = a.strip ? 1 : 0;

    uint32_t cull_mode = s.rs[kRsCullMode];
    bool cull_en = (cull_mode == 2 || cull_mode == 3 || cull_mode == 0x900 || cull_mode == 0x901) && a.topology == Topology::Triangles;
    bool cull_cw = (cull_mode == 2 || cull_mode == 0x900);
    k.cull = cull_en ? 1 : 0;
    k.cull_cw = cull_cw ? 1 : 0;

    k.blend_en = SimpleState(s, kMBlendEnable) != 0 ? 1 : 0;
    if (k.blend_en) {
        k.src_blend = SimpleState(s, kMSrcBlend);
        k.dst_blend = SimpleState(s, kMDestBlend);
        k.blend_op = SimpleState(s, kMBlendOp);
    }
    uint32_t cw = SimpleState(s, kMColorWriteEnable);
    if (cw <= 15) {
        k.wmask = cw;
    } else {
        k.wmask = ((cw >> 16) & 1) | (((cw >> 8) & 1) << 1) | ((cw & 1) << 2) | (((cw >> 24) & 1) << 3);
    }

    k.has_depth = has_depth ? 1 : 0;
    if (has_depth) {
        k.depth_en = (SimpleState(s, kMDepthTestEnable) != 0 || s.rs[kRsZEnable] != 0) ? 1 : 0;
        k.depth_write = SimpleState(s, kMZWriteEnable) != 0 ? 1 : 0;
        k.depth_func = SimpleState(s, kMZFunc) ? SimpleState(s, kMZFunc) : 4;
        k.stencil_en = (SimpleState(s, kMStencilTestEnable) != 0 || s.rs[kRsStencilEnable] != 0) ? 1 : 0;
    }

    for (int r = 0; r < 16; r++) {
        if ((a.regs >> r) & 1) {
            const VertexElement& e = a.elements[r];
            k.element_formats[r] = static_cast<uint32_t>(e.format);
            k.element_buffers[r] = e.buffer;
            k.element_offsets[r] = e.offset;
        }
    }
    for (uint32_t i = 0; i < a.buffer_count && i < 16; i++) {
        k.buffer_strides[i] = a.buffers[i].stride;
    }

    auto it = pipeline_cache_.find(k);
    if (it != pipeline_cache_.end()) return it->second;

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vs;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = ps;
    stages[1].pName = "main";

    std::vector<VkVertexInputBindingDescription> bindings;
    for (uint32_t i = 0; i < a.buffer_count; i++) {
        VkVertexInputBindingDescription bd{};
        bd.binding = i;
        bd.stride = a.buffers[i].stride;
        bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindings.push_back(bd);
    }

    std::vector<VkVertexInputAttributeDescription> attribs;
    uint32_t loc = 0;
    for (int r = 0; r < 16; r++) {
        if ((a.regs >> r) & 1) {
            const VertexElement& e = a.elements[r];
            VkVertexInputAttributeDescription ad{};
            ad.location = loc++;
            ad.binding = e.buffer;
            ad.format = DxgiToVkFormat(e.format);
            ad.offset = e.offset;
            attribs.push_back(ad);
        }
    }

    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vi.pVertexBindingDescriptions = bindings.data();
    vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
    vi.pVertexAttributeDescriptions = attribs.data();

    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = DxTopoToVk(a.topology, a.strip);
    ia.primitiveRestartEnable = a.strip ? VK_TRUE : VK_FALSE;

    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = cull_en ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rs.frontFace = cull_cw ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    if (has_depth) {
        ds.depthTestEnable = k.depth_en ? VK_TRUE : VK_FALSE;
        ds.depthWriteEnable = k.depth_write ? VK_TRUE : VK_FALSE;
        ds.depthCompareOp = VkCmp(k.depth_func);
        ds.stencilTestEnable = k.stencil_en ? VK_TRUE : VK_FALSE;
        if (k.stencil_en) {
            ds.front.compareOp = VkCmp(SimpleState(s, kMStencilFunc));
            ds.front.compareMask = SimpleState(s, kMStencilMask) & 0xFF;
            ds.front.writeMask = SimpleState(s, kMStencilWriteMask) & 0xFF;
            ds.front.reference = SimpleState(s, kMStencilRef) & 0xFF;
            ds.front.failOp = VkStencilOperation(s.rs[kRsStencilFail]);
            ds.front.depthFailOp = VkStencilOperation(SimpleState(s, kMStencilZFail));
            ds.front.passOp = VkStencilOperation(SimpleState(s, kMStencilPass));
            ds.back = ds.front;
        }
    }

    VkPipelineColorBlendAttachmentState cba{};
    cba.blendEnable = k.blend_en ? VK_TRUE : VK_FALSE;
    if (k.blend_en) {
        cba.srcColorBlendFactor = VkBlend(k.src_blend, false);
        cba.dstColorBlendFactor = VkBlend(k.dst_blend, false);
        if (cba.dstColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA_SATURATE) cba.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        cba.colorBlendOp = VkBlendOperation(k.blend_op);
        cba.srcAlphaBlendFactor = VkBlend(k.src_blend, true);
        if (cba.srcAlphaBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA_SATURATE) cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cba.dstAlphaBlendFactor = VkBlend(k.dst_blend, true);
        if (cba.dstAlphaBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA_SATURATE) cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cba.alphaBlendOp = VkBlendOperation(k.blend_op);
    }
    cba.colorWriteMask = 0;
    if (k.wmask & 1) cba.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
    if (k.wmask & 2) cba.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
    if (k.wmask & 4) cba.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
    if (k.wmask & 8) cba.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb_state.attachmentCount = 1;
    cb_state.pAttachments = &cba;

    VkDynamicState dyn_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE
    };
    VkPipelineDynamicStateCreateInfo dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(std::size(dyn_states));
    dynamic_state.pDynamicStates = dyn_states;

    VkGraphicsPipelineCreateInfo pci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pci.stageCount = 2;
    pci.pStages = stages;
    pci.pVertexInputState = &vi;
    pci.pInputAssemblyState = &ia;
    pci.pViewportState = &vp;
    pci.pRasterizationState = &rs;
    pci.pMultisampleState = &ms;
    pci.pDepthStencilState = &ds;
    pci.pColorBlendState = &cb_state;
    pci.pDynamicState = &dynamic_state;
    pci.layout = draw_pipeline_layout_;
    pci.renderPass = has_depth ? target_rp_depth_ : target_rp_color_;
    pci.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult res = vkCreateGraphicsPipelines(device_, pipeline_cache_obj_, 1, &pci, nullptr, &pipeline);
    if (res != VK_SUCCESS) {
        LOGR("vulkan: vkCreateGraphicsPipelines failed res=%d", res);
        return VK_NULL_HANDLE;
    }

    pipeline_cache_[k] = pipeline;
    return pipeline;
}

void VulkanBackend::Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) {
    BeginFrame();
    static uint64_t draw_count = 0;
    draw_count++;

    Assembled a;
    if (!Assemble(s, prim, start, count, indices, a)) return;

    VulkanTarget* rt = EnsureTarget(s.rt, false);
    if (!rt) return;
    rt->drawn = true;
    rt->epoch = DirtyEpoch(rt->data);

    VulkanTarget* dt = nullptr;
    if (s.zs.data || s.zs.header) {
        dt = EnsureTarget(s.zs, true);
        if (dt) dt->drawn = true;
    }

    bool has_depth = (dt != nullptr);
    VkFramebuffer fb = EnsureFramebuffer(rt, dt);

    VkCommandBuffer cb = command_buffers_[current_frame_];

    // 1. Textures & Samplers (Processed BEFORE render pass activation)
    TexInfo stages[4]{};
    VkImageView stage_views[4] = {dummy_view_, dummy_view_, dummy_view_, dummy_view_};
    VkSampler stage_samplers[4] = {dummy_sampler_, dummy_sampler_, dummy_sampler_, dummy_sampler_};

    for (int i = 0; i < 4; i++) {
        if (s.textures[i]) {
            TextureDesc d;
            if (Describe(s.textures[i], d)) {
                stages[i].kind = TextureKind(d);
                stages[i].rect = d.linear;

                // Check if render target
                uint64_t rt_key = uint64_t(d.data);
                auto tit = targets_.find(rt_key);
                if (tit != targets_.end() && tit->second.image != VK_NULL_HANDLE && !d.cube && d.depth == 1 && !Format(d.format).compressed) {
                    if (tit->second.layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                        if (active_rp_ != VK_NULL_HANDLE) {
                            vkCmdEndRenderPass(cb);
                            active_rp_ = VK_NULL_HANDLE;
                            active_fb_ = VK_NULL_HANDLE;
                        }
                        TransitionImageLayout(cb, tit->second.image, tit->second.layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                        tit->second.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                    stage_views[i] = tit->second.view;
                } else {
                    VulkanTexture* vt = GetOrCreateTexture(s, i, d);
                    if (vt) {
                        stage_views[i] = vt->view;
                    }
                }
            }
            const auto& t = s.tss[i];
            stage_samplers[i] = GetOrCreateSampler(t[kTssAddressU], t[kTssAddressV], t[kTssMinFilter], t[kTssMagFilter]);
        }
    }

    // 2. Shaders (Fast key-based cache without string allocations in hot path)
    bool sh_prog = (s.vs_handle & 1) && s.vs && !s.vs->program.empty();
    if (sh_prog && !s.vs->hash) {
        s.vs->hash = Hash(s.vs->program.data(), s.vs->program.size() * 4) | 1;
    }

    constexpr uint64_t kDeclarationSalt = 0x10000;
    constexpr uint64_t kPretransformedSalt = 0x7E00000000ull;
    uint64_t seed = sh_prog ? (s.vs->hash ^ a.regs) : Hash(&s.vs_handle, 4, a.regs | kDeclarationSalt);
    uint64_t vs_key = Hash(&a.decode, 8, seed);
    FixedKey fk{};
    if (!sh_prog) {
        if (a.pretransformed) vs_key ^= kPretransformedSalt;
        fk = MakeFixedKey(s);
        vs_key = Hash(&fk, sizeof(fk), vs_key);
    }

    PixelKey pk = MakePixelKey(s, stages);
    constexpr uint64_t kPixelKeySalt = 0x55AA55AA00000000ull;
    uint64_t ps_key = Hash(&pk, sizeof(pk)) ^ kPixelKeySalt;

    auto vs_it = shader_cache_.find(vs_key);
    VkShaderModule vs_mod = VK_NULL_HANDLE;
    if (vs_it != shader_cache_.end()) {
        vs_mod = vs_it->second;
    } else {
        std::string vs_hlsl = sh_prog ? VertexProgramHlsl(s.vs->program, a.regs, a.decode)
                                      : FixedVertexHlsl(a.regs, a.pretransformed, a.decode, fk);
        vs_mod = GetOrCreateShader(vs_hlsl, L"main", L"vs_6_0", vs_key);
    }

    auto ps_it = shader_cache_.find(ps_key);
    VkShaderModule ps_mod = VK_NULL_HANDLE;
    if (ps_it != shader_cache_.end()) {
        ps_mod = ps_it->second;
    } else {
        std::string ps_hlsl = PixelHlsl(pk);
        ps_mod = GetOrCreateShader(ps_hlsl, L"main", L"ps_6_0", ps_key);
    }

    if (!vs_mod || !ps_mod) return;

    // 3. Pipeline
    VkPipeline pso = GetOrCreatePipeline(s, a, vs_mod, ps_mod, has_depth);
    if (!pso) return;

    // 4. Manage render pass active state
    if (active_fb_ != fb || active_rp_ == VK_NULL_HANDLE) {
        if (active_rp_ != VK_NULL_HANDLE) {
            vkCmdEndRenderPass(cb);
            active_rp_ = VK_NULL_HANDLE;
            active_fb_ = VK_NULL_HANDLE;
        }

        TransitionImageLayout(cb, rt->image, rt->layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        rt->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (dt) {
            VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (dt->format == VK_FORMAT_D32_SFLOAT_S8_UINT || dt->format == VK_FORMAT_D24_UNORM_S8_UINT || dt->format == VK_FORMAT_D16_UNORM_S8_UINT) {
                aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            TransitionImageLayout(cb, dt->image, dt->layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, aspect);
            dt->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpbi.renderPass = has_depth ? target_rp_depth_ : target_rp_color_;
        rpbi.framebuffer = fb;
        rpbi.renderArea.offset = {0, 0};
        rpbi.renderArea.extent = {rt->w, rt->h};
        rpbi.clearValueCount = 0;
        rpbi.pClearValues = nullptr;

        vkCmdBeginRenderPass(cb, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        active_rp_ = rpbi.renderPass;
        active_fb_ = fb;
    }

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pso);

    // 5. Constants
    // VC (b0)
    static uint64_t s_last_vsc_epoch = 0;
    static uint32_t s_last_vsc_offset = 0;
    static uint32_t s_last_vsc_frame = 0xFFFFFFFF;
    uint32_t vc_offset = 0;
    if (s_last_vsc_epoch == s.vsc_epoch && s_last_vsc_frame == current_frame_) {
        vc_offset = s_last_vsc_offset;
    } else {
        DynamicAlloc vc_alloc = AllocDynamic(sizeof(s.vsc), min_uniform_alignment_);
        std::memcpy(vc_alloc.host_ptr, s.vsc.data(), sizeof(s.vsc));
        vc_offset = vc_alloc.offset;
        s_last_vsc_epoch = s.vsc_epoch;
        s_last_vsc_offset = vc_offset;
        s_last_vsc_frame = current_frame_;
    }

    // DC (b1)
    DrawConstants c{};
    GuestExtent size{rt->gw, rt->gh};
    FillSurfaceConstants(s, size, c);
    FillFogConstants(s, sh_prog, c);
    FillCombinerConstants(s, c);
    FillTextureConstants(s, c);
    if (!sh_prog && !a.pretransformed) {
        FillTransformConstants(s, size, c);
        FillLightConstants(s, fk, c);
    }
    static DrawConstants s_last_c{};
    static uint32_t s_last_dc_offset = 0;
    static uint32_t s_last_dc_frame = 0xFFFFFFFF;
    uint32_t dc_offset = 0;
    if (s_last_dc_frame == current_frame_ && std::memcmp(&s_last_c, &c, sizeof(c)) == 0) {
        dc_offset = s_last_dc_offset;
    } else {
        DynamicAlloc dc_alloc = AllocDynamic(sizeof(c), min_uniform_alignment_);
        std::memcpy(dc_alloc.host_ptr, &c, sizeof(c));
        dc_offset = dc_alloc.offset;
        s_last_c = c;
        s_last_dc_offset = dc_offset;
        s_last_dc_frame = current_frame_;
    }

    // 6. Descriptor set
    VkDescriptorSet ds = GetOrCreateDescriptorSet(stage_views, stage_samplers);
    uint32_t dyn_offsets[2] = {vc_offset, dc_offset};
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, draw_pipeline_layout_, 0, 1, &ds, 2, dyn_offsets);

    // 7. Viewport (with negative height Y-flip for DirectX compatibility) and Scissor
    float W = float(rt->w);
    float H = float(rt->h);
    VkViewport vp{};
    vp.x = 0.0f;
    vp.y = H;
    vp.width = W;
    vp.height = -H;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(cb, 0, 1, &vp);

    float sc = W / float(rt->gw);
    int32_t x0 = int32_t(s.viewport.x * sc);
    int32_t y0 = int32_t(s.viewport.y * sc);
    int32_t x1 = int32_t((s.viewport.x + s.viewport.w) * sc);
    int32_t y1 = int32_t((s.viewport.y + s.viewport.h) * sc);
    if (s.scissor_count) {
        x0 = std::max(x0, int32_t(s.scissors[0] * sc));
        y0 = std::max(y0, int32_t(s.scissors[1] * sc));
        x1 = std::min(x1, int32_t(s.scissors[2] * sc));
        y1 = std::min(y1, int32_t(s.scissors[3] * sc));
    }
    VkRect2D scissor{};
    scissor.offset.x = std::clamp<int32_t>(x0, 0, int32_t(W));
    scissor.offset.y = std::clamp<int32_t>(y0, 0, int32_t(H));
    scissor.extent.width = uint32_t(std::max<int32_t>(0, std::clamp<int32_t>(x1, 0, int32_t(W)) - scissor.offset.x));
    scissor.extent.height = uint32_t(std::max<int32_t>(0, std::clamp<int32_t>(y1, 0, int32_t(H)) - scissor.offset.y));
    vkCmdSetScissor(cb, 0, 1, &scissor);

    // Dynamic blend color & stencil ref
    float blend_col[4];
    Color(SimpleState(s, kMBlendColor), blend_col);
    vkCmdSetBlendConstants(cb, blend_col);
    vkCmdSetStencilReference(cb, VK_STENCIL_FACE_FRONT_AND_BACK, SimpleState(s, kMStencilRef) & 0xFF);

    // 6. Vertex Buffers
    VkBuffer vb_handles[16];
    VkDeviceSize vb_offsets[16];
    for (uint32_t i = 0; i < a.buffer_count; i++) {
        const VertexBuffer& b = a.buffers[i];
        DynamicAlloc vb_alloc = AllocDynamic(b.bytes, 16);
        std::memcpy(vb_alloc.host_ptr, b.src, b.bytes);
        vb_handles[i] = vb_alloc.buffer;
        vb_offsets[i] = vb_alloc.offset;
    }
    if (a.buffer_count > 0) {
        vkCmdBindVertexBuffers(cb, 0, a.buffer_count, vb_handles, vb_offsets);
    }

    // 7. Index Buffer & Draw
    if (a.index_count) {
        uint32_t ibytes = a.index_count * (a.index16 ? 2 : 4);
        DynamicAlloc ib_alloc = AllocDynamic(ibytes, 4);
        std::memcpy(ib_alloc.host_ptr, a.index_src, ibytes);
        vkCmdBindIndexBuffer(cb, ib_alloc.buffer, ib_alloc.offset, a.index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cb, a.index_count, 1, 0, a.base_vertex, 0);
    } else {
        vkCmdDraw(cb, a.draw_count, 1, 0, 0);
    }

    if (draw_count <= 3) {
        LOGR("vulkan: Draw #%llu prim=%u verts=%u indices=%u vs_mod=%p ps_mod=%p rt=0x%08X",
             (unsigned long long)draw_count, prim, a.vertex_count, a.index_count, vs_mod, ps_mod, rt->data);
        LOGR("vulkan: Draw #%llu blend=%d (src=%u dst=%u op=%u) wmask=0x%X zen=%d (write=%d func=%u) dt=%p",
             (unsigned long long)draw_count, SimpleState(s, kMBlendEnable), SimpleState(s, kMSrcBlend), SimpleState(s, kMDestBlend), SimpleState(s, kMBlendOp), SimpleState(s, kMColorWriteEnable),
             SimpleState(s, kMDepthTestEnable), SimpleState(s, kMZWriteEnable), SimpleState(s, kMZFunc), dt);
        LOGR("vulkan: Draw #%llu tex0=0x%08X (view=%p dummy=%p) tex1=0x%08X tex2=0x%08X tex3=0x%08X",
             (unsigned long long)draw_count, s.textures[0], stage_views[0], dummy_view_, s.textures[1], s.textures[2], s.textures[3]);
        if (a.buffer_count > 0 && a.buffers[0].src && a.buffers[0].bytes >= 16) {
            const float* f = reinterpret_cast<const float*>(a.buffers[0].src);
            LOGR("vulkan: Draw #%llu verts[0]: %g %g %g %g | verts[1]: %g %g %g %g",
                 (unsigned long long)draw_count, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
        }
        std::string dbg_ps = PixelHlsl(pk);
        LOGR("vulkan: Draw #%llu ps_hlsl:\n%s", (unsigned long long)draw_count, dbg_ps.c_str());
    }
}

void VulkanBackend::Copy(const Surface& src, const Surface& dst, uint32_t count, uint32_t src_rects_va, uint32_t dst_points_va) {
    static uint64_t copy_count = 0;
    if (++copy_count <= 10) {
        LOGR("vulkan: Copy #%llu count=%u", (unsigned long long)copy_count, count);
    }
    if (active_rp_ != VK_NULL_HANDLE) {
        VkCommandBuffer cb = command_buffers_[current_frame_];
        vkCmdEndRenderPass(cb);
        active_rp_ = VK_NULL_HANDLE;
        active_fb_ = VK_NULL_HANDLE;
    }
    CopySurfaceMemory(src, dst, count, src_rects_va, dst_points_va);
}

void VulkanBackend::Present(const Surface& front) {
    static uint64_t pres_count = 0;
    pres_count_ = ++pres_count;

    BeginFrame();
    VkCommandBuffer cb = command_buffers_[current_frame_];
    if (active_rp_ != VK_NULL_HANDLE) {
        vkCmdEndRenderPass(cb);
        active_rp_ = VK_NULL_HANDLE;
        active_fb_ = VK_NULL_HANDLE;
    }

    VkResult res = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_sem_[current_frame_],
                                         VK_NULL_HANDLE, &image_index_);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {  // changed under us between the last frame and this acquire
        RecreateSwapchain();
        res = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_sem_[current_frame_],
                                    VK_NULL_HANDLE, &image_index_);
    }
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        // drop the frame but leave the frame fence signalled: the next BeginFrame waits on it
        vkEndCommandBuffer(cb);
        VkSubmitInfo empty{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        vkQueueSubmit(queue_, 1, &empty, in_flight_fences_[current_frame_]);
        frame_begun_ = false;
        return;
    }

    // Check if target was drawn on GPU and if CPU has modified guest memory
    uint32_t front_data = mem::VirtualFromPhysical(front.data);
    uint64_t front_key = uint64_t(front_data);
    auto tit = targets_.find(front_key);
    bool cpu_modified = false;
    if (tit != targets_.end()) {
        uint64_t e = DirtyEpoch(front_data);
        if (e > tit->second.epoch) {
            tit->second.epoch = e;
            tit->second.drawn = false;
            cpu_modified = true;
        }
    }

    bool has_image = false;

    if (tit != targets_.end() && tit->second.image != VK_NULL_HANDLE && tit->second.drawn && !cpu_modified) {
        VulkanTarget& t = tit->second;
        TransitionImageLayout(cb, t.image, t.layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        t.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        TransitionImageLayout(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkClearColorValue black_clear{};
        black_clear.float32[3] = 1.0f;
        VkImageSubresourceRange srr{};
        srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srr.levelCount = 1;
        srr.layerCount = 1;
        vkCmdClearColorImage(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black_clear, 1, &srr);

        bool wide = app::Widescreen();
        float aspect = wide ? 16.0f / 9.0f : 4.0f / 3.0f;
        float sw = float(swapchain_extent_.width);
        float sh = float(swapchain_extent_.height);
        float vw = std::min(sw, sh * aspect);
        float vh = vw / aspect;
        int32_t vx = int32_t((sw - vw) / 2.0f);
        int32_t vy = int32_t((sh - vh) / 2.0f);

        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {int32_t(t.w), int32_t(t.h), 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstOffsets[0] = {vx, vy, 0};
        blit.dstOffsets[1] = {vx + int32_t(vw), vy + int32_t(vh), 1};

        vkCmdBlitImage(cb, t.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       swapchain_images_[image_index_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit, VK_FILTER_LINEAR);

        TransitionImageLayout(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        has_image = true;
    } else {
        uint32_t fw = 640, fh = 480;
        const uint8_t* src_px = nullptr;
        std::vector<uint8_t> decoded_px;
        uint32_t pitch = 0;

        if (front.header) {
            TextureDesc d;
            if (Describe(front.header, d)) {
                fw = d.width;
                fh = d.height;
                if (d.data) {
                    if (d.linear) {
                        pitch = d.pitch ? d.pitch : (fw * 4);
                        src_px = static_cast<const uint8_t*>(kernel::HostPtr(d.data));
                    } else {
                        DecodeLevel(d, 0, 0, static_cast<const uint8_t*>(kernel::HostPtr(d.data)), nullptr, decoded_px, pitch);
                        if (!decoded_px.empty()) {
                            src_px = decoded_px.data();
                        }
                    }
                }
            }
        }
        if (!src_px && front.data) {
            SurfaceSize(front, fw, fh);
            pitch = fw * 4;
            src_px = static_cast<const uint8_t*>(kernel::HostPtr(front_data));
        }

        if (pres_count <= 10) {
            LOGR("vulkan: Present CPU upload #%llu header=0x%08X data=0x%08X (src_px=%p %ux%u pitch=%u)",
                 (unsigned long long)pres_count, front.header, front.data, src_px, fw, fh, pitch);
        }

        if (src_px && fw > 0 && fh > 0) {
            size_t required_bytes = size_t(fw) * fh * 4;
            if (EnsureStagingBuffer(required_bytes) && EnsureFrontImage(fw, fh)) {
                if (pitch == fw * 4) {
                    std::memcpy(staging_mapped_, src_px, required_bytes);
                } else {
                    for (uint32_t y = 0; y < fh; y++) {
                        std::memcpy(static_cast<uint8_t*>(staging_mapped_) + size_t(y) * fw * 4,
                                    src_px + size_t(y) * pitch, fw * 4);
                    }
                }

                TransitionImageLayout(cb, front_image_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkBufferImageCopy region{};
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.layerCount = 1;
                region.imageExtent = {fw, fh, 1};
                vkCmdCopyBufferToImage(cb, staging_buffer_, front_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                TransitionImageLayout(cb, front_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
                TransitionImageLayout(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                VkClearColorValue black_clear{};
                black_clear.float32[3] = 1.0f;
                VkImageSubresourceRange srr{};
                srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                srr.levelCount = 1;
                srr.layerCount = 1;
                vkCmdClearColorImage(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black_clear, 1, &srr);

                bool wide = app::Widescreen();
                float aspect = wide ? 16.0f / 9.0f : 4.0f / 3.0f;
                float sw = float(swapchain_extent_.width);
                float sh = float(swapchain_extent_.height);
                float vw = std::min(sw, sh * aspect);
                float vh = vw / aspect;
                int32_t vx = int32_t((sw - vw) / 2.0f);
                int32_t vy = int32_t((sh - vh) / 2.0f);

                VkImageBlit blit{};
                blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {int32_t(fw), int32_t(fh), 1};
                blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                blit.dstOffsets[0] = {vx, vy, 0};
                blit.dstOffsets[1] = {vx + int32_t(vw), vy + int32_t(vh), 1};

                vkCmdBlitImage(cb, front_image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               swapchain_images_[image_index_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit, VK_FILTER_LINEAR);

                TransitionImageLayout(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                has_image = true;
            }
        }
    }

    if (!has_image) {
        TransitionImageLayout(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkClearColorValue black_clear{};
        black_clear.float32[3] = 1.0f;
        VkImageSubresourceRange srr{};
        srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srr.levelCount = 1;
        srr.layerCount = 1;
        vkCmdClearColorImage(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black_clear, 1, &srr);
        TransitionImageLayout(cb, swapchain_images_[image_index_], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpbi.renderPass = render_pass_;
    rpbi.framebuffer = framebuffers_[image_index_];
    rpbi.renderArea.extent = swapchain_extent_;
    rpbi.clearValueCount = 0;
    rpbi.pClearValues = nullptr;

    vkCmdBeginRenderPass(cb, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    pass_active_ = true;

    // Render ImGui overlay
    {
        std::lock_guard lock(app::UiLock());
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (app::kGame.ui) app::kGame.ui(fps_);
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
    }

    vkCmdEndRenderPass(cb);
    pass_active_ = false;
    vkEndCommandBuffer(cb);

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkSemaphore wait_sems[] = {image_available_sem_[current_frame_]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = wait_sems;
    si.pWaitDstStageMask = wait_stages;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    VkSemaphore signal_sems[] = {render_finished_sem_[current_frame_]};
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = signal_sems;

    vkQueueSubmit(queue_, 1, &si, in_flight_fences_[current_frame_]);

    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = signal_sems;
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain_;
    pi.pImageIndices = &image_index_;

    vkQueuePresentKHR(queue_, &pi);
    CountFps();
    ApplySettings();

    // the window said it changed size (WM_SIZE): follow the surface with the frame already submitted,
    // so a stale swapchain cannot crop the image into the window; one driver query per resize, not per frame
    if (g_window_resized.exchange(false, std::memory_order_relaxed)) {
        VkSurfaceCapabilitiesKHR caps{};
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &caps) == VK_SUCCESS &&
            caps.currentExtent.width != 0 && caps.currentExtent.height != 0) {
            bool changed = (caps.currentExtent.width != UINT32_MAX && caps.currentExtent.width != swapchain_extent_.width) ||
                           (caps.currentExtent.height != UINT32_MAX && caps.currentExtent.height != swapchain_extent_.height);
            if (changed) RecreateSwapchain();
        }
    }
    if ((pres_count_ % 600) == 0) SaveCachesAsync();

    current_frame_ = (current_frame_ + 1) % kMaxFrames;
    frame_begun_ = false;
}

void VulkanBackend::ApplySettings() {
    float want = app::RenderScale(guest_h_);
    if (std::fabs(want - scale_) <= 1e-4f) return;
    if (device_) vkDeviceWaitIdle(device_);
    // stale framebuffers would key the recycled view handles of the new targets
    for (auto& [k, fb] : target_fb_cache_) {
        if (fb) vkDestroyFramebuffer(device_, fb, nullptr);
    }
    target_fb_cache_.clear();
    for (auto& [k, t] : targets_) {
        if (t.view) vkDestroyImageView(device_, t.view, nullptr);
        if (t.image) vkDestroyImage(device_, t.image, nullptr);
        if (t.memory) vkFreeMemory(device_, t.memory, nullptr);
    }
    targets_.clear();
    bound_rt_ = bound_ds_ = nullptr;
    active_rp_ = VK_NULL_HANDLE;
    active_fb_ = VK_NULL_HANDLE;
    // descriptor sets point at the destroyed views: drop them all (idle, so nothing in flight)
    ds_cache_.clear();
    std::memset(ds_last_views_, 0, sizeof(ds_last_views_));
    std::memset(ds_last_samplers_, 0, sizeof(ds_last_samplers_));
    ds_last_ds_ = VK_NULL_HANDLE;
    if (draw_desc_pool_ && vkResetDescriptorPool) vkResetDescriptorPool(device_, draw_desc_pool_, 0);
    scale_ = want;
    LOGR("vulkan: render scale now %.3fx", scale_);
}

void VulkanBackend::Sync(uint32_t, uint32_t) {
    if (device_) vkDeviceWaitIdle(device_);
}

void VulkanBackend::Flush() {
    if (queue_) vkQueueWaitIdle(queue_);
}

void VulkanBackend::SetGamma(const uint8_t* ramp) {
    std::memcpy(ramp_, ramp, sizeof(ramp_));
}

void VulkanBackend::RecreateSwapchain() {
    if (!device_) return;
    vkDeviceWaitIdle(device_);
    for (auto fb : framebuffers_) vkDestroyFramebuffer(device_, fb, nullptr);
    for (auto iv : swapchain_views_) vkDestroyImageView(device_, iv, nullptr);
    if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);

    CreateSwapchain();
    CreateFramebuffers();
}

void VulkanBackend::Cleanup() {
    if (device_) {
        vkDeviceWaitIdle(device_);
        ImGui_ImplVulkan_Shutdown();
        if (imgui_pool_) vkDestroyDescriptorPool(device_, imgui_pool_, nullptr);
        for (auto& [k, p] : pipeline_cache_) {
            if (p) vkDestroyPipeline(device_, p, nullptr);
        }
        pipeline_cache_.clear();
        if (pipeline_cache_obj_) {
            size_t cache_size = 0;
            if (vkGetPipelineCacheData && vkGetPipelineCacheData(device_, pipeline_cache_obj_, &cache_size, nullptr) == VK_SUCCESS && cache_size > 0) {
                std::vector<uint8_t> cache_bytes(cache_size);
                if (vkGetPipelineCacheData(device_, pipeline_cache_obj_, &cache_size, cache_bytes.data()) == VK_SUCCESS) {
                    std::ofstream cache_out("vk_pipeline.cache", std::ios::binary);
                    if (cache_out) {
                        cache_out.write(reinterpret_cast<const char*>(cache_bytes.data()), cache_bytes.size());
                        cache_out.close();
                        LOGR("vulkan: saved %zu bytes to vk_pipeline.cache", cache_bytes.size());
                    }
                }
            }
            vkDestroyPipelineCache(device_, pipeline_cache_obj_, nullptr);
            pipeline_cache_obj_ = VK_NULL_HANDLE;
        }
        for (auto& [k, fb] : target_fb_cache_) {
            if (fb) vkDestroyFramebuffer(device_, fb, nullptr);
        }
        target_fb_cache_.clear();
        if (target_rp_depth_) {
            vkDestroyRenderPass(device_, target_rp_depth_, nullptr);
            target_rp_depth_ = VK_NULL_HANDLE;
        }
        if (target_rp_color_) {
            vkDestroyRenderPass(device_, target_rp_color_, nullptr);
            target_rp_color_ = VK_NULL_HANDLE;
        }
        for (auto& [k, t] : textures_) {
            if (t.view) vkDestroyImageView(device_, t.view, nullptr);
            if (t.image) vkDestroyImage(device_, t.image, nullptr);
            if (t.memory) vkFreeMemory(device_, t.memory, nullptr);
        }
        textures_.clear();
        for (auto& [k, s] : sampler_cache_) {
            if (s) vkDestroySampler(device_, s, nullptr);
        }
        sampler_cache_.clear();
        if (dummy_sampler_) {
            vkDestroySampler(device_, dummy_sampler_, nullptr);
            dummy_sampler_ = VK_NULL_HANDLE;
        }
        if (dummy_view_) {
            vkDestroyImageView(device_, dummy_view_, nullptr);
            dummy_view_ = VK_NULL_HANDLE;
        }
        if (dummy_image_) {
            vkDestroyImage(device_, dummy_image_, nullptr);
            dummy_image_ = VK_NULL_HANDLE;
        }
        if (dummy_memory_) {
            vkFreeMemory(device_, dummy_memory_, nullptr);
            dummy_memory_ = VK_NULL_HANDLE;
        }
        if (draw_desc_pool_) {
            vkDestroyDescriptorPool(device_, draw_desc_pool_, nullptr);
            draw_desc_pool_ = VK_NULL_HANDLE;
        }
        if (draw_ds_layout_) {
            vkDestroyDescriptorSetLayout(device_, draw_ds_layout_, nullptr);
            draw_ds_layout_ = VK_NULL_HANDLE;
        }
        if (draw_pipeline_layout_) {
            vkDestroyPipelineLayout(device_, draw_pipeline_layout_, nullptr);
            draw_pipeline_layout_ = VK_NULL_HANDLE;
        }
        for (auto& [k, m] : shader_cache_) {
            if (m) vkDestroyShaderModule(device_, m, nullptr);
        }
        shader_cache_.clear();
        SaveSpirvCache();
        spirv_cache_.clear();
        if (dynamic_mapped_) {
            vkUnmapMemory(device_, dynamic_memory_);
            dynamic_mapped_ = nullptr;
        }
        if (dynamic_buffer_) {
            vkDestroyBuffer(device_, dynamic_buffer_, nullptr);
            dynamic_buffer_ = VK_NULL_HANDLE;
        }
        if (dynamic_memory_) {
            vkFreeMemory(device_, dynamic_memory_, nullptr);
            dynamic_memory_ = VK_NULL_HANDLE;
        }
        if (staging_mapped_) {
            vkUnmapMemory(device_, staging_memory_);
            staging_mapped_ = nullptr;
        }
        if (staging_buffer_) {
            vkDestroyBuffer(device_, staging_buffer_, nullptr);
            staging_buffer_ = VK_NULL_HANDLE;
        }
        if (staging_memory_) {
            vkFreeMemory(device_, staging_memory_, nullptr);
            staging_memory_ = VK_NULL_HANDLE;
        }
        if (front_image_) {
            vkDestroyImage(device_, front_image_, nullptr);
            front_image_ = VK_NULL_HANDLE;
        }
        if (front_image_memory_) {
            vkFreeMemory(device_, front_image_memory_, nullptr);
            front_image_memory_ = VK_NULL_HANDLE;
        }
        for (auto f : in_flight_fences_) vkDestroyFence(device_, f, nullptr);
        for (auto s : render_finished_sem_) vkDestroySemaphore(device_, s, nullptr);
        for (auto s : image_available_sem_) vkDestroySemaphore(device_, s, nullptr);
        if (command_pool_) vkDestroyCommandPool(device_, command_pool_, nullptr);
        for (auto fb : framebuffers_) vkDestroyFramebuffer(device_, fb, nullptr);
        if (render_pass_) vkDestroyRenderPass(device_, render_pass_, nullptr);
        for (auto iv : swapchain_views_) vkDestroyImageView(device_, iv, nullptr);
        if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    if (surface_) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (instance_) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

}  // namespace

std::unique_ptr<Backend> CreateVulkanBackend() {
    auto b = std::make_unique<VulkanBackend>();
    return b;
}

}  // namespace gpu
