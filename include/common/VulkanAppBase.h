#pragma once

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

    class VulkanAppBase {
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
        VkPipelineCache pipelineCache{};

        VmaAllocator vmaAllocator;
        std::vector<VkFramebuffer> frameBuffers{};


        bool init();

        void shutdown();

        bool prepare();

        void initWindow();

        VkResult createInstance();

        void createCommandPool();

        bool createCommandBuffers();

        void createSynchronizationPrimitives();

        void setupDepthStencil();

        void setupRenderPass();

        void createPipelineCache();

        void setupFrameBuffer();

    private:

    };

}