#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>


namespace ref_vk {

    typedef struct SSwapChainBuffer {
        VkImage image;
        VkImageView view;
    } TSwapChainBuffer;

    class CSwapChain {
    private:
        VkInstance instance;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkSurfaceKHR surface;

    public:
        VkFormat colorFormat;
        VkColorSpaceKHR colorSpace;
        VkSwapchainKHR swapchain{};
        uint32_t imageCount;
        std::vector<VkImage> images{};
        std::vector<TSwapChainBuffer> buffers;
        uint32_t queueNodeIndex = UINT32_MAX;

        void setContext(VkInstance instance, VkPhysicalDevice phyDevice, VkDevice logicDevice);

    };

}
