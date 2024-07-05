#include <common/CTools.h>
#include <fstream>

namespace REF_VK {

    void LOG(LOG_ERRORS type, const char *text) {
        std::cout << "Log ";
        switch (type) {
            case LOG_ERRORS::NORMAL:
                std::cout << "Normal: " << text << "\n";
                break;

            case LOG_ERRORS::ERR:
                std::cout << "Error: " << text << "\n";
                break;

            case LOG_ERRORS::DEBUG:
                std::cout << "Debug: " << text << "\n";
                break;

            default:
                std::cout << "Unknown: " << text << "\n";
                break;
        }
    }

    bool VK_CHECK_RESULT(VkResult res) {
        if (res != VK_SUCCESS) {
            LOG(DEBUG, "Function execute error for vulkan!");
            return false;
        }
        return true;
    }

    bool VK_CHECK_RESULT(VkResult res, const char *text) {
        if (res != VK_SUCCESS) {
            LOG(DEBUG, text);
            return false;
        }
        return true;
    }

    VkBool32 getSupportedDepthStencilFormat(VkPhysicalDevice phyDevice, VkFormat *depthStencilFormat) {
        std::vector<VkFormat> formatList{
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
        };

        for (auto &format: formatList) {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(phyDevice, format, &formatProps);
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                *depthStencilFormat = format;
                return true;
            }
        }

        return false;
    }

    VkBool32 getSupportedDepthFormat(VkPhysicalDevice phyDevice, VkFormat *depthFormat) {
        std::vector<VkFormat> formatList{
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM
        };

        for (auto &format: formatList) {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(phyDevice, format, &formatProps);
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                *depthFormat = format;
                return true;
            }
        }

        return false;
    }

    VkCommandBufferAllocateInfo
    genCommandBufferAllocateInfo(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t bufferCount) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = level;
        commandBufferAllocateInfo.commandBufferCount = bufferCount;
        return commandBufferAllocateInfo;
    }

    VkFenceCreateInfo genFenceCreateInfo(VkFenceCreateFlags flags) {
        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = flags;
        return fenceCI;
    }

    VkShaderModule loadSPIRVShader(VkDevice device, std::string fileName) {
        size_t shaderSize{};
        char *shaderCode{};

        std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

        if (is.is_open()) {
            shaderSize = is.tellg();
            is.seekg(0, std::ios::beg);

            // Copy data to buffer
            shaderCode = new char[shaderSize];
            is.read(shaderCode, shaderSize);
            is.close();
            assert(shaderSize > 0);
        }

        if (shaderCode) {
            VkShaderModuleCreateInfo shaderModuleCI{};
            shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleCI.codeSize = shaderSize;
            shaderModuleCI.pCode = (uint32_t *) shaderCode;

            VkShaderModule shaderModule{};
            VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule),
                            ("Cannot create shader module! \n\t SPIR-V file path = " + fileName).c_str());

            delete[] shaderCode;

            return shaderModule;
        } else {
            VK_CHECK_RESULT(VkResult::VK_ERROR_UNKNOWN,
                            ("Cannot read shader file! \n\t SPIR-V file path = " + fileName).c_str());
            return VK_NULL_HANDLE;
        }

        return nullptr;
    }

    std::string getBasedAssetsPath() {
        return "../Assets/";
    }
}
