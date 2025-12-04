// Wyn GPU Runtime - Vulkan Backend (Cross-platform)
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static VkInstance instance = VK_NULL_HANDLE;
static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static VkDevice device = VK_NULL_HANDLE;
static VkQueue queue = VK_NULL_HANDLE;

// Initialize Vulkan
void __wyn_gpu_init() {
    if (instance != VK_NULL_HANDLE) return;
    
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Wyn GPU";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Wyn";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
        return;
    }
    
    // Get physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount > 0) {
        VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
        physicalDevice = devices[0];
        free(devices);
    }
    
    // Create logical device
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkDeviceCreateInfo deviceCreateInfo = {0};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) == VK_SUCCESS) {
        vkGetDeviceQueue(device, 0, 0, &queue);
    }
}

// Get device count
int __wyn_gpu_device_count() {
    __wyn_gpu_init();
    uint32_t deviceCount = 0;
    if (instance != VK_NULL_HANDLE) {
        vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    }
    return (int)deviceCount;
}

// Get device name
const char* __wyn_gpu_device_name(int device_id) {
    __wyn_gpu_init();
    static char name[256];
    
    if (physicalDevice != VK_NULL_HANDLE) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        strncpy(name, props.deviceName, 255);
        name[255] = '\0';
        return name;
    }
    
    return "Vulkan Device";
}

// Allocate GPU memory
void* __wyn_gpu_malloc(size_t size) {
    __wyn_gpu_init();
    
    if (device == VK_NULL_HANDLE) {
        return malloc(size); // Fallback to CPU
    }
    
    VkBuffer buffer;
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(device, &bufferInfo, NULL, &buffer) != VK_SUCCESS) {
        return malloc(size); // Fallback
    }
    
    // Allocate memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0; // Simplified
    
    VkDeviceMemory memory;
    if (vkAllocateMemory(device, &allocInfo, NULL, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, NULL);
        return malloc(size); // Fallback
    }
    
    vkBindBufferMemory(device, buffer, memory, 0);
    
    // Return buffer handle (simplified)
    return (void*)(uintptr_t)buffer;
}

// Free GPU memory
void __wyn_gpu_free(void* ptr) {
    if (device != VK_NULL_HANDLE && ptr) {
        VkBuffer buffer = (VkBuffer)(uintptr_t)ptr;
        vkDestroyBuffer(device, buffer, NULL);
    } else {
        free(ptr);
    }
}

// Synchronize
void __wyn_gpu_sync() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }
}

// Copy to device (simplified)
void __wyn_gpu_memcpy_to_device(void* dst, const void* src, size_t size) {
    // Simplified - in production would use staging buffers
    memcpy(dst, src, size);
}

// Copy from device (simplified)
void __wyn_gpu_memcpy_from_device(void* dst, const void* src, size_t size) {
    // Simplified - in production would use staging buffers
    memcpy(dst, src, size);
}

// Launch kernel (simplified)
void __wyn_gpu_launch_kernel(const char* kernel_source, void** buffers, int buffer_count, int grid_size) {
    // Kernel execution would require compute pipeline setup
    // Simplified for now
}

