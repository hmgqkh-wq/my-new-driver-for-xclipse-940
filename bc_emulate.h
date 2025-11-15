#pragma once
#include <vulkan/vulkan.h>
#ifdef __cplusplus
extern "C" {
#endif

void bc_init(VkInstance instance);
int bc_register_compressed_image(VkDevice device, VkImage image, VkFormat format, uint32_t width, uint32_t height);
int bc_force_decompress(VkDevice device, VkImage image);
int bc_schedule_decompress_on_first_use(VkDevice device, VkImage image);
void bc_shutdown();

#ifdef __cplusplus
}
#endif
