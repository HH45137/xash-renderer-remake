#include <common/CDevice.h>
#include <vulkan/vulkan_core.h>
#include <common/CTools.h>


namespace ref_vk {
    CDevice::CDevice(VkPhysicalDevice phyDevice) {
        assert(phyDevice);
        this->physicalDevice = phyDevice;

        vkGetPhysicalDeviceProperties(phyDevice, &this->properties);
        vkGetPhysicalDeviceFeatures(phyDevice, &this->features);
        vkGetPhysicalDeviceMemoryProperties(phyDevice, &this->memoryProperties);

        // Queue family
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, nullptr);
        assert(queueFamilyCount > 0);
        this->queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, this->queueFamilyProperties.data());

        //List of supported extensions
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &extCount, nullptr);
        if (extCount > 0) {
            std::vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &extCount, &extensions.front()) ==
                VK_SUCCESS) {
                for (auto &ext: extensions) {
                    this->supportedExtensions.push_back(ext.extensionName);
                }
            }
        }

        return;
    }

    CDevice::~CDevice() {
        if (commandPool) {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }
        if (device) {
            vkDestroyDevice(device, nullptr);
        }
    }

    VkResult
    CDevice::createLogicalDevice(VkPhysicalDeviceFeatures enableFeatures, std::vector<const char *> enableExtensions,
                                 void *pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes) {

        std::vector<VkDeviceQueueCreateInfo> queueCreateInofs{};

        const float defaultQueuePriority = 0.0f;

        // Graphics queue
        if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInofs.push_back(queueInfo);
        } else {
            queueFamilyIndices.graphics = 0;
        }

        // Compute queue
        if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInofs.push_back(queueInfo);
        } else {
            queueFamilyIndices.compute = queueFamilyIndices.graphics;
        }

        // Transfer queue
        if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
            queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInofs.push_back(queueInfo);
        } else {
            queueFamilyIndices.transfer = queueFamilyIndices.graphics;
        }

        // Create logical device
        std::vector<const char *> deviceExtensions(enableExtensions);

        // Use swapchain?
        if (useSwapChain) {
            deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInofs.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInofs.data();
        deviceCreateInfo.pEnabledFeatures = &enableFeatures;

        VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};

        // Use nextchain?
        if (pNextChain) {
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.features = enableFeatures;
            physicalDeviceFeatures2.pNext = pNextChain;
            deviceCreateInfo.pEnabledFeatures = nullptr;
            deviceCreateInfo.pNext = &physicalDeviceFeatures2;
        }

        if (deviceExtensions.size() > 0) {
            for (auto enableExtension: deviceExtensions) {
                if (!extensionSupported(enableExtension)) {
                    std::cout << "Enabled device extension: \"" << enableExtension << "\"." << "\n";
                }
            }

            deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        }

        this->enableFeatures = enableFeatures;

        VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            return result;
        }

        commandPool = createCommandPool(queueFamilyIndices.graphics);

        return result;
    }

    uint32_t CDevice::getQueueFamilyIndex(VkQueueFlags queueFlags) {

        if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags) {
            for (int i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i) {
                if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                    ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                    return i;
                }
            }
        }

        if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags) {
            for (int i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i) {
                if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                    ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
                    ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
                    return i;
                }
            }
        }

        for (int i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i) {
            if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags) {
                return i;
            }
        }

        throw std::runtime_error("Could not find a queue family index");
    }

    bool CDevice::extensionSupported(std::string extension) {
        return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) !=
                supportedExtensions.end());
    }

    VkCommandPool CDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags) const {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
        cmdPoolInfo.flags = createFlags;
        VkCommandPool cmdPool;
        VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));

        return cmdPool;
    }
}
