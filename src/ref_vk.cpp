#define VMA_IMPLEMENTATION

#include <ref_vk.h>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <common/VulkanDebug.h>
#include <common/CTools.h>
#include <common/CDevice.h>
#include <common/CSwapChain.h>
#include <common/vk_mem_alloc.h>

#if defined(_WIN32)

#include <Windows.h>
#include <vulkan/vulkan_win32.h>

#endif

namespace REF_VK {

#if defined(_DEBUG)
    bool enabledValidationLayer = true;
#else
    bool enabledValidationLayer = false;
#endif

    class CRef_Vk {
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
        REF_VK::CDevice *device{};
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
        std::vector<VkCommandBuffer> drawCmdBuffers{};
        // Vertex buffer
        struct {
            VkBuffer buffer;
            VmaAllocation allocation;
        } vertices;
        //Indices buffer
        struct {
            VkBuffer buffer;
            VmaAllocation allocation;
            uint32_t count;
        } indices;
        VkQueue queue{};
        std::vector<VkFence> waitFences{};
        struct {
            VkImage image;
            VmaAllocation allocation;
            VkImageView imageView;
        } depthStencil;
        VkRenderPass renderPass{};

        VmaAllocator vmaAllocator;


        bool init() {
            bool isSuccess = true;

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
            SDL_Log("%s", SDL_GetError());

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

            //Find a suitable depth, stencil format
            VkBool32 validFormat{};
            if (requiresStencil) {
                validFormat = REF_VK::getSupportedDepthStencilFormat(phyDevice, &depthFormat);
            } else {
                validFormat = REF_VK::getSupportedDepthFormat(phyDevice, &depthFormat);
            }
            assert(validFormat);

            // prepare swap chain context
            swapChain.setContext(instance, phyDevice, logicDevice);

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

            // Initial swap chain surface
            isSuccess = swapChain.initSurface(window);

            // Create command pool
            VkCommandPoolCreateInfo cmdPoolCI{};
            cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolCI.queueFamilyIndex = swapChain.queueNodeIndex;
            cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            VK_CHECK_RESULT(vkCreateCommandPool(logicDevice, &cmdPoolCI, nullptr, &cmdPool),
                            "Create command pool error!");

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

            // Create vertex buffer
//            createVertexBuffer();

            return isSuccess;
        }

        void shutdown() {
            SDL_DestroyWindow(window);
            SDL_Vulkan_UnloadLibrary();
            SDL_Quit();
            SDL_Log("%s", SDL_GetError());
        }

    private:
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

        bool createCommandBuffers() {

            drawCmdBuffers.resize(swapChain.imageCount);
            auto CI = genCommandBufferAllocateInfo(cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                   static_cast<uint32_t>(drawCmdBuffers.size()));
            bool result = VK_CHECK_RESULT(vkAllocateCommandBuffers(logicDevice, &CI, drawCmdBuffers.data()));
            return result;
        }

        void createSynchronizationPrimitives() {
            // Wait fences to sync command buffer access
            VkFenceCreateInfo fenceCI = genFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
            waitFences.resize(drawCmdBuffers.size());
            for (auto &fence: waitFences) {
                VK_CHECK_RESULT(vkCreateFence(logicDevice, &fenceCI, nullptr, &fence));
            }
        }

        void setupDepthStencil() {
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

        void setupRenderPass() {
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

        void createVertexBuffer() {

            // Vertex data
            std::vector<Vertex> vertexBuffer{
                    {{1.0f,  1.0f,  0.0f}, {1.0f, 0.0f, 0.0f}},
                    {{-1.0f, 1.0f,  0.0f}, {0.0f, 1.0f, 0.0f}},
                    {{0.0f,  -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}
            };
            uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size() * sizeof(Vertex));

            // Index data
            std::vector<uint32_t> indexBuffer{0,
                                              1,
                                              2};
            uint32_t indexBufferSize = static_cast<uint32_t>(indexBuffer.size() * sizeof(uint32_t));
            indices.count = indexBufferSize;

            // Temp staging buffer
            struct SStagingBuffer {
                VkBuffer buffer;
                VmaAllocation allocation;
            };
            struct {
                SStagingBuffer vertices;
                SStagingBuffer indices;
            } stagingBuffers{};

            // VMA memory usage auto
            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

            // Allocator vertex staging buffer memory
            VkBufferCreateInfo vbCI{};
            vbCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vbCI.size = vertexBufferSize;
            vbCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            vbCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VmaAllocationInfo stagingVertexBufferAllocInfo{};
            bool result = VK_CHECK_RESULT(
                    vmaCreateBuffer(vmaAllocator, &vbCI, &allocInfo, &stagingBuffers.vertices.buffer,
                                    &stagingBuffers.vertices.allocation,
                                    &stagingVertexBufferAllocInfo));
            // Map and copy
            void *data = stagingVertexBufferAllocInfo.pMappedData;
//            vmaMapMemory(vmaAllocator, stagingBuffers.vertices.allocation, &data);
            memcpy(data, vertexBuffer.data(), vertexBufferSize);
//            vmaUnmapMemory(vmaAllocator, stagingBuffers.vertices.allocation);
            // Allocator vertex buffer memory
            vbCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            vbCI.flags = 0;
            vmaCreateBuffer(vmaAllocator, &vbCI, &allocInfo, &vertices.buffer, &vertices.allocation, nullptr);

            // Allocator index staging buffer memory
            VkBufferCreateInfo ibCI{};
            ibCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            ibCI.size = indexBufferSize;
            ibCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            ibCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VmaAllocationInfo stagingIndexBufferAllocInfo{};
            result = VK_CHECK_RESULT(
                    vmaCreateBuffer(vmaAllocator, &ibCI, &allocInfo, &stagingBuffers.indices.buffer,
                                    &stagingBuffers.indices.allocation,
                                    &stagingIndexBufferAllocInfo));
            // Map and copy
            data = stagingIndexBufferAllocInfo.pMappedData;
//            vmaMapMemory(vmaAllocator, stagingBuffers.indices.allocation, &data);
            memcpy(data, indexBuffer.data(), indexBufferSize);
//            vmaUnmapMemory(vmaAllocator, stagingBuffers.indices.allocation);
            // Allocator index buffer memory
            ibCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            ibCI.flags = 0;
            vmaCreateBuffer(vmaAllocator, &ibCI, &allocInfo, &indices.buffer, &indices.allocation, nullptr);


            //Copy command buffer submit
            VkCommandBuffer copyCmdBuf;

            VkCommandBufferAllocateInfo cmdBuffAllocateInfo{};
            cmdBuffAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBuffAllocateInfo.commandPool = cmdPool;
            cmdBuffAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBuffAllocateInfo.commandBufferCount = 1;
            VK_CHECK_RESULT(vkAllocateCommandBuffers(logicDevice, &cmdBuffAllocateInfo, &copyCmdBuf));

            // Start command
            VkCommandBufferBeginInfo cmdBuffBeginInfo{};
            cmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmdBuf, &cmdBuffBeginInfo));
            {
                VkBufferCopy copyRegion{};
                // Vertex buffer copy
                copyRegion.size = vertexBufferSize;
                vkCmdCopyBuffer(copyCmdBuf, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
                // Index buffer copy
                copyRegion.size = indexBufferSize;
                vkCmdCopyBuffer(copyCmdBuf, stagingBuffers.indices.buffer, indices.buffer, 1, &copyRegion);
            }
            // Stop command
            VK_CHECK_RESULT(vkEndCommandBuffer(copyCmdBuf));

            // Create fence for wait command buffer executed
            VkFenceCreateInfo fenceCI{};
            fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCI.flags = 0;
            VkFence fence;
            VK_CHECK_RESULT(vkCreateFence(logicDevice, &fenceCI, nullptr, &fence));

            // Submit to queue
            VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
            // Wait...
            VK_CHECK_RESULT(vkWaitForFences(logicDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

            vkDestroyFence(logicDevice, fence, nullptr);
            vkFreeCommandBuffers(logicDevice, cmdPool, 1, &copyCmdBuf);

            vmaDestroyBuffer(vmaAllocator, stagingBuffers.vertices.buffer, stagingBuffers.vertices.allocation);
            vmaFreeMemory(vmaAllocator, stagingBuffers.vertices.allocation);
            vmaDestroyBuffer(vmaAllocator, stagingBuffers.indices.buffer, stagingBuffers.indices.allocation);
            vmaFreeMemory(vmaAllocator, stagingBuffers.indices.allocation);

            return;
        }

    };

    CRef_Vk ref_vk_obj{};
}

REF_VK::qboolean R_Init(void) {
    REF_VK::qboolean result = REF_VK::ref_vk_obj.init();
    return result;
}

void R_Shutdown(void) {
    REF_VK::ref_vk_obj.shutdown();
}

const char *R_GetConfigName(void) {
    return "vulkan";
}
