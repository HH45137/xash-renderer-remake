#include <common/CSwapChain.h>
#include <common/CTools.h>


namespace ref_vk {

    void CSwapChain::setContext(VkInstance instance, VkPhysicalDevice phyDevice, VkDevice logicDevice) {
        this->instance = instance;
        this->physicalDevice = phyDevice;
        this->device = logicDevice;
    }

    bool CSwapChain::initSurface(SDL_Window *window) {

        SDL_Vulkan_CreateSurface(window, this->instance, &this->surface);
        if (!this->surface) {
            LOG(ERR, "Could not create surface!");
            return false;
        }

        // Get available queue family properties
        uint32_t queueCount{};
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
        assert(queueCount >= 1);
        std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProperties.data());

        // Find suitable queue for present
        std::vector<VkBool32> supportsPresent(queueCount);
        for (int i = 0; i < queueCount; ++i) {
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);
        }

        // Find support for graphics and present queue.
        uint32_t graphicsQueueNodeIndex = UINT32_MAX;
        uint32_t presentQueueNodeIndex = UINT32_MAX;
        for (int i = 0; i < queueCount; ++i) {
            if ((queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && ((supportsPresent[i] == true))) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
        if ((graphicsQueueNodeIndex == UINT32_MAX && presentQueueNodeIndex == UINT32_MAX) &&
            (graphicsQueueNodeIndex == presentQueueNodeIndex)) {
            LOG(ERR, "Could not find a suitable graphics and present queue!");
            return false;
        }
        queueNodeIndex = graphicsQueueNodeIndex;

        // Get supported surface formats
        uint32_t formatCount{};
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr),
                        "Not find suitable surface formats!");
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        std::vector<VkFormat> preferredImageFormats{
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_B8G8R8A8_UNORM,
                VK_FORMAT_A8B8G8R8_UNORM_PACK32
        };
        VkSurfaceFormatKHR selectFormat = surfaceFormats[0];
        for (auto &availableFormat: surfaceFormats) {
            if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) !=
                preferredImageFormats.end()) {
                selectFormat = availableFormat;
                break;
            }
        }
        this->colorFormat = selectFormat.format;
        this->colorSpace = selectFormat.colorSpace;

        return true;
    }

}
