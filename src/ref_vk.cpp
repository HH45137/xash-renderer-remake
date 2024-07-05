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
#include <common/VulkanAppBase.h>
#include <glm/glm.hpp>

#if defined(_WIN32)

#include <Windows.h>
#include <vulkan/vulkan_win32.h>

#endif

#define MAX_CONCURRENT_FRAMES 2


namespace REF_VK {

    class CRef_Vk : public VulkanAppBase {
    public:
        void createSynchronizationPrimitives();

        void createCommandBuffers();

        void createVertexBuffer();

        void createUniformBuffers();

        void createDescriptorSetLayout();

        void createDescriptorPool();

        void createDescriptorSets();

        bool prepare();

    private:
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

        typedef struct SShaderData {
            glm::mat4 projection;
            glm::mat4 model;
            glm::mat4 view;
        } TShaderData;

        typedef struct SUniformBuffer {
            VmaAllocation allocation;
            VkBuffer buffer;
            // store the resources bound to the binding point in a shader
            VkDescriptorSet descriptorSet;
            uint8_t *mapped{nullptr};
        } TUniformBuffer;

        std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> presentCompleteSemaphores{};
        std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> renderCompleteSemaphores{};
        std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};
        VkCommandPool commandPool{};
        std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};
        std::array<TUniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers{};
        VkDescriptorSetLayout descriptorSetLayout{};
        VkPipelineLayout pipelineLayout{};
    };

    void CRef_Vk::createSynchronizationPrimitives() {
        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_CONCURRENT_FRAMES; ++i) {
            VK_CHECK_RESULT(vkCreateSemaphore(logicDevice, &semaphoreCI, nullptr, &presentCompleteSemaphores[i]));
            VK_CHECK_RESULT(vkCreateSemaphore(logicDevice, &semaphoreCI, nullptr, &renderCompleteSemaphores[i]));

            VK_CHECK_RESULT(vkCreateFence(logicDevice, &fenceCI, nullptr, &waitFences[i]));
        }

        return;
    }

    void CRef_Vk::createVertexBuffer() {

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
        cmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
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

        // Submit the command buffer to the queue to finish the copy
        VkSubmitInfo submitInfoTemp{};
        submitInfoTemp.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfoTemp.commandBufferCount = 1;
        submitInfoTemp.pCommandBuffers = &copyCmdBuf;

        // Submit to queue
        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfoTemp, fence));
        // Wait...
        VK_CHECK_RESULT(vkWaitForFences(logicDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

        vkDestroyFence(logicDevice, fence, nullptr);
        vkFreeCommandBuffers(logicDevice, cmdPool, 1, &copyCmdBuf);

        vmaDestroyBuffer(vmaAllocator, stagingBuffers.vertices.buffer, stagingBuffers.vertices.allocation);
//        vmaFreeMemory(vmaAllocator, stagingBuffers.vertices.allocation);
        vmaDestroyBuffer(vmaAllocator, stagingBuffers.indices.buffer, stagingBuffers.indices.allocation);
//        vmaFreeMemory(vmaAllocator, stagingBuffers.indices.allocation);

        return;
    }

    bool CRef_Vk::prepare() {
        VulkanAppBase::prepare();
        createSynchronizationPrimitives();
        createCommandBuffers();
        createVertexBuffer();
        createUniformBuffers();
        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSets();

        return true;
    }

    void CRef_Vk::createCommandBuffers() {
        VkCommandPoolCreateInfo commandPoolCI{};
        commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCI.queueFamilyIndex = swapChain.queueNodeIndex;
        commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK_RESULT(vkCreateCommandPool(logicDevice, &commandPoolCI, nullptr, &commandPool));

        VkCommandBufferAllocateInfo cmdBufAllocateInfo = genCommandBufferAllocateInfo(commandPool,
                                                                                      VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                                                      MAX_CONCURRENT_FRAMES);
        VK_CHECK_RESULT(vkAllocateCommandBuffers(logicDevice, &cmdBufAllocateInfo, commandBuffers.data()));
    }

    void CRef_Vk::createUniformBuffers() {

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = sizeof(TShaderData);
        // This buffer type is The Uniform buffer
        bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        for (int i = 0; i < MAX_CONCURRENT_FRAMES; ++i) {
            VK_CHECK_RESULT(vmaCreateBuffer(vmaAllocator, &bufferCI, &allocInfo, &uniformBuffers[i].buffer,
                                            &uniformBuffers[i].allocation,
                                            nullptr),
                            "Cannot create Uniform buffer!");
            VK_CHECK_RESULT(
                    vmaMapMemory(vmaAllocator, uniformBuffers[i].allocation, (void **) &uniformBuffers[i].mapped),
                    "Cannot map Uniform buffer!");
        }

        return;
    }

    void CRef_Vk::createDescriptorSetLayout() {

        // Binding 0: Uniform buffer (Vertex shader)
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
        descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCI.pNext = nullptr;
        descriptorSetLayoutCI.bindingCount = 1;
        descriptorSetLayoutCI.pBindings = &layoutBinding;
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(logicDevice, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout),
                        "Cannot create descriptor set layout");

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.pNext = nullptr;
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
        VK_CHECK_RESULT(vkCreatePipelineLayout(logicDevice, &pipelineLayoutCI, nullptr, &pipelineLayout),
                        "Cannot create pipeline layout");

        return;
    }

    void CRef_Vk::createDescriptorPool() {
        VkDescriptorPoolSize descriptorPoolSize[1]{};
        descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorPoolSize[0].descriptorCount = MAX_CONCURRENT_FRAMES;

        VkDescriptorPoolCreateInfo descriptorPoolCI{};
        descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCI.pNext = nullptr;
        descriptorPoolCI.poolSizeCount = 1;
        descriptorPoolCI.pPoolSizes = descriptorPoolSize;
        descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;

        VK_CHECK_RESULT(vkCreateDescriptorPool(logicDevice, &descriptorPoolCI, nullptr, &descriptorPool),
                        "Cannot create descriptor pool!");

        return;
    }

    void CRef_Vk::createDescriptorSets() {
        // Allocate one descriptor set per frame from the global descriptor pool
        for (int i = 0; i < MAX_CONCURRENT_FRAMES; ++i) {
            VkDescriptorSetAllocateInfo allocateInfo{};
            allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocateInfo.descriptorPool = descriptorPool;
            allocateInfo.descriptorSetCount = 1;
            allocateInfo.pSetLayouts = &descriptorSetLayout;
            VK_CHECK_RESULT(vkAllocateDescriptorSets(logicDevice, &allocateInfo, &uniformBuffers[i].descriptorSet));

            // Update the descriptor set determining the shader binding points
            VkWriteDescriptorSet writeDescriptorSet{};

            // The buffers information is passed using a descriptor info structure
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].buffer;
            bufferInfo.range = sizeof(TShaderData);

            // Binding 0 : Uniform buffer
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.pBufferInfo = &bufferInfo;
            writeDescriptorSet.dstBinding = 0;
            vkUpdateDescriptorSets(logicDevice, 1, &writeDescriptorSet, 0, nullptr);

            return;
        }

    }

    CRef_Vk ref_vk_obj{};
}

REF_VK::qboolean R_Init(void) {
    REF_VK::qboolean result = REF_VK::ref_vk_obj.init();
    result = REF_VK::ref_vk_obj.prepare();
    return result;
}

void R_Shutdown(void) {
    REF_VK::ref_vk_obj.shutdown();
}

const char *R_GetConfigName(void) {
    return "vulkan";
}
