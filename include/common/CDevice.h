#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>


namespace ref_vk {
    class CDevice {
    public:
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceFeatures enableFeatures;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        std::vector<std::string> supportedExtensions;
        VkCommandPool commandPool = VK_NULL_HANDLE;

        struct {
            uint32_t graphics;
            uint32_t compute;
            uint32_t transfer;
        } queueFamilyIndices;

        explicit CDevice(VkPhysicalDevice phyDevice);

        ~CDevice();

        VkResult createLogicalDevice(VkPhysicalDeviceFeatures enableFeatures, std::vector<std::string> enableExtensions,
                                     void *pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes);

    };

}
