\
// Layer core with command-buffer interception to schedule decompression on first use.
// This file provides the plumbing to record a small "decompress job" into the application's command buffers.
// Notes: Recording from a layer must carefully preserve application sync; this implementation demonstrates
// a conservative approach: on first-use, submit a small command buffer on the application's queue that performs
// decompression into the backing image before the app continues. Production integration should rewrite command buffers.

#include "bc_emulate.h"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <mutex>
#include <cstdio>
#include <cstring>

#define LOGI(fmt, ...) fprintf(stdout, "[ExynosFull] " fmt "\n", ##__VA_ARGS__)

static PFN_vkGetInstanceProcAddr real_gipa = nullptr;
static PFN_vkGetDeviceProcAddr real_gdpa = nullptr;

struct ImgRec {
    VkImage appImage;
    VkImage backingImage;
    VkFormat format;
    uint32_t w,h;
    bool decompressed;
};

static std::mutex g_lock;
static std::unordered_map<uint64_t, ImgRec> g_images;

extern "C" PFN_vkVoidFunction VKAPI_CALL hooked_vkGetInstanceProcAddr(VkInstance instance, const char* pName);
extern "C" PFN_vkVoidFunction VKAPI_CALL hooked_vkGetDeviceProcAddr(VkDevice device, const char* pName);

static uint64_t key_from_handle(void* h){ return (uint64_t)(uintptr_t)h; }

// Intercept vkGetPhysicalDeviceFormatProperties to advertise compressed formats
VKAPI_ATTR void VKAPI_CALL hooked_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice phys, VkFormat format, VkFormatProperties* pProps) {
    static PFN_vkGetPhysicalDeviceFormatProperties real = nullptr;
    if(!real) real = (PFN_vkGetPhysicalDeviceFormatProperties)real_gipa(VK_NULL_HANDLE, "vkGetPhysicalDeviceFormatProperties");
    if(real) real(phys, format, pProps);
    if(format >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && format <= VK_FORMAT_BC7_SRGB_BLOCK) {
        pProps->linearTilingFeatures |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        pProps->optimalTilingFeatures |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    }
}

// Create backing image and register
VKAPI_ATTR VkResult VKAPI_CALL hooked_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAlloc, VkImage* pImage) {
    static PFN_vkCreateImage real = nullptr;
    if(!real) real = (PFN_vkCreateImage)real_gdpa(VK_NULL_HANDLE, "vkCreateImage");
    if(!real) return VK_ERROR_INITIALIZATION_FAILED;

    if(pCreateInfo && pCreateInfo->format >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && pCreateInfo->format <= VK_FORMAT_BC7_SRGB_BLOCK) {
        VkImageCreateInfo newInfo = *pCreateInfo;
        newInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        VkResult r = real(device, &newInfo, pAlloc, pImage);
        if(r==VK_SUCCESS) {
            // register mapping
            bc_register_compressed_image(device, *pImage, pCreateInfo->format, pCreateInfo->extent.width, pCreateInfo->extent.height);
            LOGI("Registered compressed image for lazy-emulation");
        }
        return r;
    }
    return real(device, pCreateInfo, pAlloc, pImage);
}

// A conservative interception point: when vkQueueSubmit is called, check for first-use and schedule decompression.
// This approach inserts a small submit before the app continue, ensuring the backingImage is ready.
VKAPI_ATTR VkResult VKAPI_CALL hooked_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    static PFN_vkQueueSubmit real = nullptr;
    if(!real) real = (PFN_vkQueueSubmit)real_gdpa(VK_NULL_HANDLE, "vkQueueSubmit");
    // For brevity: scanning application's command buffers to detect sampled usage is complex.
    // Here we call into bc_schedule_decompress_on_first_use which will check registered images and ensure decompress as needed.
    // The real implementation would parse command buffers and insert necessary barriers; this function demonstrates the hook.
    // Schedule decompression for any registered but not-yet-decompressed images bound to device of this queue.
    // (Production: this must respect sync and queue families.)
    bc_schedule_decompress_on_first_use(VK_NULL_HANDLE, VK_NULL_HANDLE);
    return real(queue, submitCount, pSubmits, fence);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL hooked_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    if(!real_gdpa) real_gdpa = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkGetDeviceProcAddr");
    if(strcmp(pName, "vkCreateImage")==0) return (PFN_vkVoidFunction)hooked_vkCreateImage;
    if(strcmp(pName, "vkQueueSubmit")==0) return (PFN_vkVoidFunction)hooked_vkQueueSubmit;
    if(strcmp(pName, "vkGetPhysicalDeviceFormatProperties")==0) return (PFN_vkVoidFunction)hooked_vkGetPhysicalDeviceFormatProperties;
    return real_gdpa(device, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL hooked_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    if(!real_gipa) real_gipa = (PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkGetInstanceProcAddr");
    if(strcmp(pName, "vkGetDeviceProcAddr")==0) return (PFN_vkVoidFunction)hooked_vkGetDeviceProcAddr;
    if(strcmp(pName, "vkGetPhysicalDeviceFormatProperties")==0) return (PFN_vkVoidFunction)hooked_vkGetPhysicalDeviceFormatProperties;
    return real_gipa(instance, pName);
}

// Export loader symbols
extern "C" {
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) { return hooked_vkGetInstanceProcAddr(instance,pName); }
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) { return hooked_vkGetDeviceProcAddr(device,pName); }
}
