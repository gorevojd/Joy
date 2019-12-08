#ifndef WIN32_VULKAN_H
#define WIN32_VULKAN_H

#include "joy_types.h"
#include "joy_memory.h"

#include <Windows.h>

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

struct vulkan_phys_device{
	VkPhysicalDevice Device;
    
	VkPhysicalDeviceFeatures Features;
	VkPhysicalDeviceProperties Props;
	VkPhysicalDeviceMemoryProperties MemProps;
    
	uint32_t QueueFamilyPropsCount;
	VkQueueFamilyProperties* QueueFamilyProps;
	int GraphicsQueueFamilyIndex;
	int ComputeQueueFamilyIndex;
};

struct vulkan_state{
	HMODULE Library;
    
	VkExtensionProperties* ExtensionProps;
	uint32_t ExtensionPropsCount;
    
	VkLayerProperties* LayerProps;
	uint32_t LayerPropsCount;
    
	VkInstance Instance;
    
	VkPhysicalDevice* PhysDevicesHandles;
	vulkan_phys_device* PhysDevices;
	uint32_t PhysDevicesCount;
    
	VkPhysicalDeviceGroupProperties* GroupProps;
	uint32_t GroupPropsCount;
};

/* NOTE(dima): 
 Vulkan API procedures can be divided into three types:
 
 Global-level functions. Allow us to create a Vulkan instance.
 Instance-level functions. Check what Vulkan-capable hardware is available and what Vulkan features are exposed.
 Device-level functions. Responsible for performing jobs typically done in a 3D application (like drawing).
*/
#define VULKAN_FUNC(func) PFN_##func func;
#define VULKAN_GET_FUNC(func, instance) func = (PFN_##func)vkGetInstanceProcAddr(instance, #func)

VULKAN_FUNC(vkGetInstanceProcAddr)

VULKAN_FUNC(vkCreateInstance)
VULKAN_FUNC(vkDestroyInstance)
VULKAN_FUNC(vkEnumerateInstanceExtensionProperties)
VULKAN_FUNC(vkEnumerateInstanceLayerProperties)

VULKAN_FUNC(vkEnumeratePhysicalDevices)
VULKAN_FUNC(vkGetPhysicalDeviceFeatures)
VULKAN_FUNC(vkGetPhysicalDeviceProperties)
VULKAN_FUNC(vkGetPhysicalDeviceQueueFamilyProperties)
VULKAN_FUNC(vkGetPhysicalDeviceMemoryProperties)
VULKAN_FUNC(vkEnumeratePhysicalDeviceGroups)
VULKAN_FUNC(vkGetDeviceProcAddr)
VULKAN_FUNC(vkCreateDevice)
VULKAN_FUNC(vkDestroyDevice)

void Win32InitVulkan(vulkan_state* Vulkan, memory_region* Mem);
void Win32CleanupVulkan(vulkan_state* Vulkan);

#endif