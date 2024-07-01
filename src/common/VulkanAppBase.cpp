#define VMA_IMPLEMENTATION

#include <common/VulkanAppBase.h>

#if defined(_DEBUG)
bool enabledValidationLayer = true;
#else
bool enabledValidationLayer = false;
#endif


void REF_VK::VulkanAppBase::setupFrameBuffer() {
    std::array<VkImageView, 2> attachments{};

    attachments[1] = depthStencil.imageView;

    VkFramebufferCreateInfo framebufferCI{};
    framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCI.pNext = nullptr;
    framebufferCI.renderPass = renderPass;
    framebufferCI.attachmentCount = 2;
    framebufferCI.pAttachments = attachments.data();
    framebufferCI.width = winWidth;
    framebufferCI.height = winHeight;
    framebufferCI.layers = 1;

    // Create frame buffers for every swap chain image
    frameBuffers.resize(swapChain.imageCount);
    for (int i = 0; i < swapChain.buffers.size(); ++i) {
        attachments[0] = swapChain.buffers[i].view;
        VK_CHECK_RESULT(vkCreateFramebuffer(logicDevice, &framebufferCI, nullptr, &frameBuffers[i]));
    }

}

void REF_VK::VulkanAppBase::createPipelineCache() {
    VkPipelineCacheCreateInfo pipelineCacheCI{};
    pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(logicDevice, &pipelineCacheCI, nullptr, &pipelineCache));
}

void REF_VK::VulkanAppBase::setupRenderPass() {
    std::array<VkAttachmentDescription, 2> attachments{};

    // Color attachment
    attachments[0].format = swapChain.colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference{};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask =
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask =
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCI.pAttachments = attachments.data();
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassCI.pDependencies = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(logicDevice, &renderPassCI, nullptr, &renderPass));

}

void REF_VK::VulkanAppBase::setupDepthStencil() {
    // Image
    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = depthFormat;
    imageCI.extent = {static_cast<uint32_t>(winWidth), static_cast<uint32_t>(winHeight), 1};
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    // Allocate memory for image and bind to out image
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    VK_CHECK_RESULT(
            vmaCreateImage(vmaAllocator, &imageCI, &allocInfo, &depthStencil.image, &depthStencil.allocation,
                           nullptr));

    // Image view
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.format = depthFormat;
    imageViewCI.subresourceRange = {};
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.image = depthStencil.image;
    VK_CHECK_RESULT(
            vkCreateImageView(logicDevice, &imageViewCI, nullptr, &depthStencil.imageView));

}

void REF_VK::VulkanAppBase::createSynchronizationPrimitives() {
    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCI = genFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    waitFences.resize(drawCmdBuffers.size());
    for (auto &fence: waitFences) {
        VK_CHECK_RESULT(vkCreateFence(logicDevice, &fenceCI, nullptr, &fence));
    }
}

bool REF_VK::VulkanAppBase::createCommandBuffers() {

    drawCmdBuffers.resize(swapChain.imageCount);
    auto CI = genCommandBufferAllocateInfo(cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                           static_cast<uint32_t>(drawCmdBuffers.size()));
    bool result = VK_CHECK_RESULT(vkAllocateCommandBuffers(logicDevice, &CI, drawCmdBuffers.data()));
    return result;
}

VkResult REF_VK::VulkanAppBase::createInstance() {
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
                supportedInstanceExtensions.push_back(extension.extensionName);
            }
        }
    }

    // Set requested instance extensions
    if (enableInstanceExtensions.size() > 0) {
        for (auto enableExtension: enableInstanceExtensions) {
            if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
                          enableExtension) == supportedInstanceExtensions.end()) {
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
        std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
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

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    // If the debug utils extension is present we set up debug functions, so samples can label objects for debugging
    if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
        vks::debugutils::setup(instance);
    }

    return result;
}

void REF_VK::VulkanAppBase::shutdown() {
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    SDL_Log("%s", SDL_GetError());
}

bool REF_VK::VulkanAppBase::init() {
    bool isSuccess = true;

    initWindow();

    /* ========== Init Vulkan ========== */

    // Create instance
    isSuccess = VK_CHECK_RESULT(createInstance());

    if (enabledValidationLayer) {
        vks::debug::setupDebugging(instance);
    }

    // Set physics device
    // Get physics device number
    uint32_t gpuCount = 0;
    isSuccess = VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
    if (gpuCount <= 0) {
        LOG(ERR, "Could not find support vulkan GPU");
        return false;
    }
    // Enumerate all physics devices
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    isSuccess = VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()),
                                "Could not enumerate physical devices"
    );

    // GPU id selection
    uint32_t selectionDeviceIndex = 0;
    // Set logical device
    phyDevice = physicalDevices[selectionDeviceIndex];
    device = new REF_VK::CDevice(phyDevice);
    isSuccess = VK_CHECK_RESULT(device->createLogicalDevice(enableFeatures, enableDeviceExtensions,
                                                            deviceCreateNextChain),
                                "Could not create vulkan device");
    logicDevice = device->device;

    // Get graphics queue form the device
    vkGetDeviceQueue(logicDevice,device->queueFamilyIndices.graphics, 0, &queue);

    //Find a suitable depth, stencil format
    VkBool32 validFormat{};
    if (requiresStencil) {
        validFormat = REF_VK::getSupportedDepthStencilFormat(phyDevice, &depthFormat);
    } else {
        validFormat = REF_VK::getSupportedDepthFormat(phyDevice, &depthFormat);
    }
    assert(validFormat);

    // Create synchronization objects
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // Create synchronization semaphore for swap chain image presentation
    VK_CHECK_RESULT(
            vkCreateSemaphore(logicDevice, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete),
            "Cannot create synchronization semaphore for swap chain image presentation");
    // Create synchronization semaphore for command buffer
    VK_CHECK_RESULT(
            vkCreateSemaphore(logicDevice, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete),
            "Cannot create synchronization semaphore for command buffer");

    // Setup synchronization objects
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &submitPipelineStageFlags;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.presentComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderComplete;

    return isSuccess;
}

void REF_VK::VulkanAppBase::initWindow() {
    /* ========== Init SDL window ========== */
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    window = SDL_CreateWindow(
            winTitle,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            winWidth,
            winHeight,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
//    SDL_Log("%s", SDL_GetError());
}

void REF_VK::VulkanAppBase::createCommandPool() {
    VkCommandPoolCreateInfo cmdPoolCI{};
    cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCI.queueFamilyIndex = swapChain.queueNodeIndex;
    cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(logicDevice, &cmdPoolCI, nullptr, &cmdPool),
                    "Create command pool error!");
}

bool REF_VK::VulkanAppBase::prepare() {
    // prepare swap chain context
    swapChain.setContext(instance, phyDevice, logicDevice);

    // Initial swap chain surface
    bool isSuccess = swapChain.initSurface(window);

    // Create command pool
    createCommandPool();

    // Setup swap chain
    swapChain.create(&winWidth, &winHeight, false, false);

    // Create VMA
    VmaAllocatorCreateInfo vmaAllocatorCI{};
    vmaAllocatorCI.device = this->logicDevice;
    vmaAllocatorCI.instance = this->instance;
    vmaAllocatorCI.physicalDevice = this->phyDevice;
    vmaCreateAllocator(&vmaAllocatorCI, &this->vmaAllocator);

    // Create command buffers
    createCommandBuffers();

    createSynchronizationPrimitives();

    setupDepthStencil();

    setupRenderPass();

    createPipelineCache();

    setupFrameBuffer();

    return isSuccess;
}
