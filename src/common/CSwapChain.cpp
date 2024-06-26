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
        VK_CHECK_RESULT(
                vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

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

    void CSwapChain::create(int *width, int *height, bool vsync, bool fullscreen) {

        assert(this->physicalDevice);
        assert(this->device);
        assert(this->instance);

        VkSwapchainKHR oldSwapchain = swapchain;

        VkSurfaceCapabilitiesKHR surfCaps{};
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps),
                        "Cannot get surface properties!");

        // Get supported present modes
        uint32_t presentModeCount{};
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr),
                        "Cannot get supported present mode count!");
        assert(presentModeCount > 0);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount,
                                                                  presentModes.data()),
                        "Cannot get supported present modes!");

        // Sync width and height
        VkExtent2D swapchainExtent{};
        if (surfCaps.currentExtent.width == (uint32_t) -1) {
            swapchainExtent.width = *width;
            swapchainExtent.height = *height;
        } else {
            swapchainExtent = surfCaps.currentExtent;
            *width = static_cast<int>(surfCaps.currentExtent.width);
            *height = static_cast<int>(surfCaps.currentExtent.height);
        }

        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        // if not VSYNC
        if (!vsync) {
            for (int i = 0; i < presentModeCount; ++i) {
                if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                    swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                    swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                    break;
                }
            }
        }

        // Determine the number of images
        uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
        if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) {
            desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
        }

        // Find transformation of the surface
        VkSurfaceTransformFlagsKHR preTransform{};
        if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        } else {
            preTransform = surfCaps.currentTransform;
        }

        // Find supported alpha format
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags{
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };
        for (auto &compositeAlphaFlag: compositeAlphaFlags) {
            if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
                compositeAlpha = compositeAlphaFlag;
                break;
            }
        }

        // Create swap chain
        VkSwapchainCreateInfoKHR swapchainCI{};
        swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCI.surface = surface;
        swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
        swapchainCI.imageFormat = colorFormat;
        swapchainCI.imageColorSpace = colorSpace;
        swapchainCI.imageExtent = {swapchainExtent.width, swapchainExtent.height};
        swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR) preTransform;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.queueFamilyIndexCount = 0;
        swapchainCI.presentMode = swapchainPresentMode;
        swapchainCI.oldSwapchain = oldSwapchain;
        swapchainCI.clipped = VK_TRUE;
        swapchainCI.compositeAlpha = compositeAlpha;
        // If supported transfer, use it
        if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        // Create swap chain
        VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain), "Cannot create swapchain!");

        // If any swapchian is re-created, destroy the old swapchain
        if (oldSwapchain != VK_NULL_HANDLE) {
            for (int i = 0; i < imageCount; ++i) {
                vkDestroyImageView(device, buffers[i].view, nullptr);
            }
            vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
        }
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr),
                        "Cannot get swapchain image count!");

        // Create swapchain images
        images.resize(imageCount);
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()),
                        "Cannot create swapchain images!");

        // Create swapchain buffers, containing the images and imageview
        buffers.resize(imageCount);
        for (int i = 0; i < imageCount; ++i) {
            VkImageViewCreateInfo colorAttachmentView{};
            colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorAttachmentView.pNext = nullptr;
            colorAttachmentView.format = colorFormat;
            colorAttachmentView.components = {
                    VK_COMPONENT_SWIZZLE_R,
                    VK_COMPONENT_SWIZZLE_G,
                    VK_COMPONENT_SWIZZLE_B,
                    VK_COMPONENT_SWIZZLE_A
            };
            colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentView.subresourceRange.baseMipLevel = 0;
            colorAttachmentView.subresourceRange.levelCount = 1;
            colorAttachmentView.subresourceRange.baseArrayLayer = 0;
            colorAttachmentView.subresourceRange.layerCount = 1;
            colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
            colorAttachmentView.flags = 0;

            buffers[i].image = images[i];
            colorAttachmentView.image = buffers[i].image;

            VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view),
                            "Cannot create swapchain buffers, containing the images and imageview");

            return;
        }

    }

}
