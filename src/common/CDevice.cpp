#include <common/CDevice.h>


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
}
