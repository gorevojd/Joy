#include "win32_vulkan.h"

void Win32InitVulkan(vulkan_state* Vulkan, memory_region* Mem){
	/*
  TODO(dima): 
  1) Layers and extensions in InstanceCreateInfo
 */
    
	Vulkan->Library = LoadLibraryA("vulkan-1.dll");
    
	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(Vulkan->Library, "vkGetInstanceProcAddr");
    
	VULKAN_GET_FUNC(vkCreateInstance, 0);
	VULKAN_GET_FUNC(vkEnumerateInstanceExtensionProperties, 0);
	VULKAN_GET_FUNC(vkEnumerateInstanceLayerProperties, 0);
	
	vkEnumerateInstanceLayerProperties(&Vulkan->LayerPropsCount, 0);
	Vulkan->LayerProps = PushArray(Mem, VkLayerProperties, Vulkan->LayerPropsCount);
	vkEnumerateInstanceLayerProperties(&Vulkan->LayerPropsCount, Vulkan->LayerProps);
    
	vkEnumerateInstanceExtensionProperties(0, &Vulkan->ExtensionPropsCount, 0);
	Vulkan->ExtensionProps = PushArray(Mem, VkExtensionProperties, Vulkan->ExtensionPropsCount);
	vkEnumerateInstanceExtensionProperties(0, &Vulkan->ExtensionPropsCount, Vulkan->ExtensionProps);
    
	VkApplicationInfo AppInfo = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,		
		0,
		"Joy",
		VK_MAKE_VERSION(1, 0, 0),
		"Joy",
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_1
	};
    
#if 0
	//NOTE(dima): Assume we load all available extensions
	char** ToLoadExtensions = PushArray(Mem, char*, Vulkan->ExtensionPropsCount);
	for(int ExtensionIndex = 0;
		ExtensionIndex < Vulkan->ExtensionPropsCount;
		ExtensionIndex++)
	{
		ToLoadExtensions[ExtensionIndex] = Vulkan->ExtensionProps[ExtensionIndex].extensionName;
	}
#endif
    
	VkInstanceCreateInfo InstanceCreateInfo = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		0,
		0,
		&AppInfo,
		0, 
		0,
		0,
		0,
	};
    
	VkResult InstanceCreateResult = vkCreateInstance(&InstanceCreateInfo, 0, &Vulkan->Instance);
	Assert(InstanceCreateResult == VK_SUCCESS);
    
	VULKAN_GET_FUNC(vkDestroyInstance, Vulkan->Instance);
	VULKAN_GET_FUNC(vkEnumeratePhysicalDevices, Vulkan->Instance);
	VULKAN_GET_FUNC(vkGetPhysicalDeviceFeatures, Vulkan->Instance);
	VULKAN_GET_FUNC(vkGetPhysicalDeviceProperties, Vulkan->Instance);
	VULKAN_GET_FUNC(vkGetPhysicalDeviceQueueFamilyProperties, Vulkan->Instance);
	VULKAN_GET_FUNC(vkGetPhysicalDeviceMemoryProperties, Vulkan->Instance);
	VULKAN_GET_FUNC(vkCreateDevice, Vulkan->Instance);
	VULKAN_GET_FUNC(vkDestroyDevice, Vulkan->Instance);
	VULKAN_GET_FUNC(vkGetDeviceProcAddr, Vulkan->Instance);
	VULKAN_GET_FUNC(vkEnumeratePhysicalDeviceGroups, Vulkan->Instance);
    
	/* Processing physical devices */
	vkEnumeratePhysicalDevices(Vulkan->Instance, &Vulkan->PhysDevicesCount, 0);
	Vulkan->PhysDevicesHandles = PushArray(Mem, VkPhysicalDevice, Vulkan->PhysDevicesCount);
	Vulkan->PhysDevices = PushArray(Mem, vulkan_phys_device, Vulkan->PhysDevicesCount);
	vkEnumeratePhysicalDevices(Vulkan->Instance, &Vulkan->PhysDevicesCount, Vulkan->PhysDevicesHandles);
    
	for(int I = 0; I < Vulkan->PhysDevicesCount; I++){
		vulkan_phys_device* Device = Vulkan->PhysDevices + I;
        
		Device->Device = Vulkan->PhysDevicesHandles[I];
        
		vkGetPhysicalDeviceFeatures(Device->Device, &Device->Features);
		vkGetPhysicalDeviceProperties(Device->Device, &Device->Props);
        
		vkGetPhysicalDeviceQueueFamilyProperties(Device->Device, &Device->QueueFamilyPropsCount, 0);
		Device->QueueFamilyProps = PushArray(Mem, VkQueueFamilyProperties, Device->QueueFamilyPropsCount);
		vkGetPhysicalDeviceQueueFamilyProperties(Device->Device, &Device->QueueFamilyPropsCount, Device->QueueFamilyProps);
        
		b32 ShouldFindGraphics = 1;
		b32 ShouldFindCompute = 1;
		for(int Qi = 0; Qi < Device->QueueFamilyPropsCount; Qi++){
			VkQueueFamilyProperties* Props = Device->QueueFamilyProps + Qi;
            
			if(ShouldFindGraphics){
				if(Props->queueFlags & VK_QUEUE_GRAPHICS_BIT){
					Device->GraphicsQueueFamilyIndex = Qi;
                    
					ShouldFindGraphics = 0;
				}
			}
            
			if(ShouldFindCompute){
				if(Props->queueFlags & VK_QUEUE_COMPUTE_BIT){
					Device->ComputeQueueFamilyIndex = Qi;
                    
					ShouldFindCompute = 0;
				}
			}
		}
        
		if(!ShouldFindGraphics){
			//NOTE(dima): Graphics queue family found
			
		}
        
		if(!ShouldFindCompute){
			//NOTE(dima): Compute queue family found
            
		}
        
		vkGetPhysicalDeviceMemoryProperties(Device->Device, &Device->MemProps);
	}
    
#if 0
	/* Processing physical devices groups */
	vkEnumeratePhysicalDeviceGroups(Vulkan->Instance, &Vulkan->GroupPropsCount, 0);
	Vulkan->GroupProps = PushArray(Mem, VkPhysicalDeviceGroupProperties, Vulkan->GroupPropsCount);
	vkEnumeratePhysicalDeviceGroups(Vulkan->Instance, &Vulkan->GroupPropsCount, Vulkan->GroupProps);
#endif
    //float GraphicsQueuePriorities[] = {1.0f};
    //VkDeviceQueueCreateInfo GraphicsQueueCreateInfo = {
    //	VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    //	0,
    //	0,
    //
    //	ArrayCount(GraphicsQueuePriorities),
    //	GraphicsQueuePriorities
    //};
}

void Win32CleanupVulkan(vulkan_state* Vulkan){
	
	
	vkDestroyInstance(Vulkan->Instance, 0);
}
