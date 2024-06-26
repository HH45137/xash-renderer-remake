#include <ref_vk.h>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <common/VulkanDebug.h>
#include <common/CTools.h>
#include <common/CDevice.h>
#include <common/CSwapChain.h>

#if defined(_WIN32)

#include <Windows.h>
#include <vulkan/vulkan_win32.h>

#endif

namespace ref_vk {

#if defined(_DEBUG)
    bool enabledValidationLayer = true;
#else
    bool enabledValidationLayer = false;
#endif

    struct env_s {
    public:
        // Window
        SDL_Window *window;
        const char *winTitle = "Test Window";
        int winWidth = 800, winHeight = 600;

        // Vulkan
        VkInstance instance{};
        std::vector<std::string> supportedInstanceExtensions{};
        std::vector<std::string> enableInstanceExtensions{};
        VkPhysicalDevice phyDevice{};
        VkDevice logicDevice{};
        ref_vk::CDevice *device{};
        VkPhysicalDeviceFeatures enableFeatures{};
        std::vector<const char *> enableDeviceExtensions{};
        void *deviceCreateNextChain = nullptr;
        VkBool32 requiresStencil{};
        VkFormat depthFormat{};
        CSwapChain swapChain{};
        struct {
            // Swap chain image presentation
            VkSemaphore presentComplete;
            // Command buffer submit and execution
            VkSemaphore renderComplete;
        } semaphores;
        VkSubmitInfo submitInfo{};
        VkPipelineStageFlags submitPipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkCommandPool cmdPool{};
    };
    env_s r_env{};

}


using namespace ref_vk;

VkResult createInstance() {
    std::vector<const char *> instanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME};

    // Depending on os the extensions
#if defined(_WIN32)
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    // Get extensions supported by the instance and store for later use
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
            for (VkExtensionProperties &extension: extensions) {
                r_env.supportedInstanceExtensions.push_back(extension.extensionName);
            }
        }
    }

    // Set requested instance extensions
    if (r_env.enableInstanceExtensions.size() > 0) {
        for (auto enableExtension: r_env.enableInstanceExtensions) {
            if (std::find(r_env.supportedInstanceExtensions.begin(), r_env.supportedInstanceExtensions.end(),
                          enableExtension) == r_env.supportedInstanceExtensions.end()) {
                std::cout << "Enabled instance extension: " << enableExtension << "\n";
            }
            instanceExtensions.push_back(enableExtension.c_str());
        }
    }

    // ========== Create vulkan instance ==========
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Xash vulkan renderer";
    appInfo.pEngineName = "Vulkan renderer";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
    if (enabledValidationLayer) {
        vks::debug::setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
        debugUtilsMessengerCI.pNext = instanceCreateInfo.pNext;
        instanceCreateInfo.pNext = &debugUtilsMessengerCI;
    }
    if (enabledValidationLayer ||
        std::find(r_env.supportedInstanceExtensions.begin(), r_env.supportedInstanceExtensions.end(),
                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != r_env.supportedInstanceExtensions.end()) {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if (instanceExtensions.size() > 0) {
        instanceCreateInfo.enabledExtensionCount = (uint32_t) instanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    }

    // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
    // Note that on Android this layer requires at least NDK r20
    const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (enabledValidationLayer) {
        // Check if this layer is available at instance level
        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
        bool validationLayerPresent = false;
        for (VkLayerProperties &layer: instanceLayerProperties) {
            if (strcmp(layer.layerName, validationLayerName) == 0) {
                validationLayerPresent = true;
                break;
            }
        }
        if (validationLayerPresent) {
            instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
            instanceCreateInfo.enabledLayerCount = 1;
        } else {
            std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
        }
    }

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &r_env.instance);

    // If the debug utils extension is present we set up debug functions, so samples can label objects for debugging
    if (std::find(r_env.supportedInstanceExtensions.begin(), r_env.supportedInstanceExtensions.end(),
                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != r_env.supportedInstanceExtensions.end()) {
        vks::debugutils::setup(r_env.instance);
    }

    return result;
}

qboolean R_Init(void) {
    bool isSuccess = true;

    /* ========== Init SDL window ========== */
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    r_env.window = SDL_CreateWindow(
            r_env.winTitle,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            r_env.winWidth,
            r_env.winHeight,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    SDL_Log("%s", SDL_GetError());

    /* ========== Init Vulkan ========== */

    // Create instance
    isSuccess = VK_CHECK_RESULT(createInstance());

    if (enabledValidationLayer) {
        vks::debug::setupDebugging(r_env.instance);
    }

    // Set physics device
    // Get physics device number
    uint32_t gpuCount = 0;
    isSuccess = VK_CHECK_RESULT(vkEnumeratePhysicalDevices(r_env.instance, &gpuCount, nullptr));
    if (gpuCount <= 0) {
        LOG(ERR, "Could not find support vulkan GPU");
        return false;
    }
    // Enumerate all physics devices
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    isSuccess = VK_CHECK_RESULT(vkEnumeratePhysicalDevices(r_env.instance, &gpuCount, physicalDevices.data()),
                                "Could not enumerate physical devices"
    );

    // GPU id selection
    uint32_t selectionDeviceIndex = 0;
    // Set logical device
    r_env.phyDevice = physicalDevices[selectionDeviceIndex];
    r_env.device = new ref_vk::CDevice(r_env.phyDevice);
    isSuccess = VK_CHECK_RESULT(r_env.device->createLogicalDevice(r_env.enableFeatures, r_env.enableDeviceExtensions,
                                                                  r_env.deviceCreateNextChain),
                                "Could not create vulkan device");
    r_env.logicDevice = r_env.device->device;

    //Find a suitable depth, stencil format
    VkBool32 validFormat{};
    if (r_env.requiresStencil) {
        validFormat = ref_vk::getSupportedDepthStencilFormat(r_env.phyDevice, &r_env.depthFormat);
    } else {
        validFormat = ref_vk::getSupportedDepthFormat(r_env.phyDevice, &r_env.depthFormat);
    }
    assert(validFormat);

    // prepare swap chain context
    r_env.swapChain.setContext(r_env.instance, r_env.phyDevice, r_env.logicDevice);

    // Create synchronization objects
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // Create synchronization semaphore for swap chain image presentation
    VK_CHECK_RESULT(
            vkCreateSemaphore(r_env.logicDevice, &semaphoreCreateInfo, nullptr, &r_env.semaphores.presentComplete),
            "Cannot create synchronization semaphore for swap chain image presentation");
    // Create synchronization semaphore for command buffer
    VK_CHECK_RESULT(
            vkCreateSemaphore(r_env.logicDevice, &semaphoreCreateInfo, nullptr, &r_env.semaphores.renderComplete),
            "Cannot create synchronization semaphore for command buffer");

    // Setup synchronization objects
    r_env.submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    r_env.submitInfo.pWaitDstStageMask = &r_env.submitPipelineStageFlags;
    r_env.submitInfo.waitSemaphoreCount = 1;
    r_env.submitInfo.pWaitSemaphores = &r_env.semaphores.presentComplete;
    r_env.submitInfo.signalSemaphoreCount = 1;
    r_env.submitInfo.pSignalSemaphores = &r_env.semaphores.renderComplete;

    // Initial swap chain surface
    isSuccess = r_env.swapChain.initSurface(r_env.window);

    // Create command pool
    VkCommandPoolCreateInfo cmdPoolCI{};
    cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCI.queueFamilyIndex = r_env.swapChain.queueNodeIndex;
    cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(r_env.logicDevice, &cmdPoolCI, nullptr, &r_env.cmdPool),
                    "Create command pool error!");

    // Setup swap chain
    r_env.swapChain.create(&r_env.winWidth, &r_env.winHeight, false, false);

    return isSuccess;
}

void R_Shutdown(void) {
    SDL_DestroyWindow(r_env.window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    SDL_Log("%s", SDL_GetError());
}

const char *R_GetConfigName(void) {
    return "vulkan";
}
