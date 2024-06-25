#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>


namespace ref_vk {
    class CDevice {
    public:
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceFeatures enableFeatures{};
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        std::vector<VkQueueFamilyProperties> queueFamilyProperties{};
        std::vector<std::string> supportedExtensions{};
        VkCommandPool commandPool = VK_NULL_HANDLE;

        struct {
            uint32_t graphics;
            uint32_t compute;
            uint32_t transfer;
        } queueFamilyIndices;

        explicit CDevice(VkPhysicalDevice phyDevice);

        ~CDevice();

        VkResult createLogicalDevice(VkPhysicalDeviceFeatures enableFeatures, std::vector<const char*> enableExtensions,
                                     void *pNextChain, bool useSwapChain = true,
                                     VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

    private:
        uint32_t getQueueFamilyIndex(VkQueueFlags queueFlags);

        bool extensionSupported(std::string extension);

        VkCommandPool createCommandPool(uint32_t queueFamilyIndex,
                                        VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) const;

    };

}
