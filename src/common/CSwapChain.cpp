#include <common/CSwapChain.h>


namespace ref_vk {

    void CSwapChain::setContext(VkInstance instance, VkPhysicalDevice phyDevice, VkDevice logicDevice) {
        this->instance = instance;
        this->physicalDevice = phyDevice;
        this->device = logicDevice;
    }

}
