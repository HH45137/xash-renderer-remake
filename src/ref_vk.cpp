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

        void createPipelines();

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
        VkPipeline pipeline{};
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
        createPipelines();

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

    void CRef_Vk::createPipelines() {
        // ============ Create the graphics pipeline ============

        VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
        graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCI.layout = pipelineLayout;
        graphicsPipelineCI.renderPass = renderPass;

        // Used triangle mode assembled data
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
        inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
        rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
        rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCI.depthClampEnable = VK_FALSE;
        rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCI.depthBiasEnable = VK_FALSE;
        rasterizationStateCI.lineWidth = 1.0f;

        // Color blend state describes how blend factors ar e calculated;
        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.colorWriteMask = 0xf;
        blendAttachmentState.blendEnable = VK_FALSE;
        VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
        colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCI.attachmentCount = 1;
        colorBlendStateCI.pAttachments = &blendAttachmentState;

        // Viewport state sets the number of viewports and scissor used in this pipeline
        VkPipelineViewportStateCreateInfo viewportStateCI{};
        viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCI.viewportCount = 1;
        viewportStateCI.scissorCount = 1;

        // Enable dynamic states
        std::vector<VkDynamicState> dynamicStateEnables{};
        dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
        VkPipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
        dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

        // Depth and stencil state containing depth and stencil compare and test operations
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
        depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCI.depthTestEnable = VK_TRUE;
        depthStencilStateCI.depthWriteEnable = VK_TRUE;
        depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
        depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
        depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depthStencilStateCI.stencilTestEnable = VK_FALSE;
        depthStencilStateCI.front = depthStencilStateCI.back;

        // Multi sampling state
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleStateCreateInfo.pSampleMask = nullptr;

        // Vertex input descriptions

        // Vertex input binding
        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(Vertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // Input attribute bindings describe shader attribute locations and memory layouts
        std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes{};
        // Example
        //	layout (location = 0) in vec3 inPos;
        //	layout (location = 1) in vec3 inColor;

        // Attribute location 0 : Position
        vertexInputAttributes[0].binding = 0;
        vertexInputAttributes[0].location = 0;
        vertexInputAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertexInputAttributes[0].offset = offsetof(Vertex, position);
        // Attribute location 1 : Color
        vertexInputAttributes[1].binding = 0;
        vertexInputAttributes[1].location = 1;
        vertexInputAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertexInputAttributes[1].offset = offsetof(Vertex, color);

        // Vertex input state used for pipeline creation
        VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
        vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCI.vertexBindingDescriptionCount = 1;
        vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBindingDescription;
        vertexInputStateCI.vertexAttributeDescriptionCount = 2;
        vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

        // Shaders
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

        // Vertex Shader
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = loadSPIRVShader(logicDevice,
                                                 getBasedAssetsPath() + "/shaders/triangle/triangle.vert.spv");
        shaderStages[0].pName = "main";
        assert(shaderStages[0].module != VK_NULL_HANDLE);

        // Fragment Shader
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = loadSPIRVShader(logicDevice,
                                                 getBasedAssetsPath() + "/shaders/triangle/triangle.frag.spv");
        shaderStages[1].pName = "main";
        assert(shaderStages[1].module != VK_NULL_HANDLE);

        // Set pipeline shader stage info
        graphicsPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        graphicsPipelineCI.pStages = shaderStages.data();

        // Set pipeline states info
        graphicsPipelineCI.pVertexInputState = &vertexInputStateCI;
        graphicsPipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
        graphicsPipelineCI.pRasterizationState = &rasterizationStateCI;
        graphicsPipelineCI.pColorBlendState = &colorBlendStateCI;
        graphicsPipelineCI.pMultisampleState = &multisampleStateCreateInfo;
        graphicsPipelineCI.pViewportState = &viewportStateCI;
        graphicsPipelineCI.pDepthStencilState = &depthStencilStateCI;
        graphicsPipelineCI.pDynamicState = &dynamicStateCI;

        // Create rendering pipeline
        VK_CHECK_RESULT(
                vkCreateGraphicsPipelines(logicDevice, pipelineCache, 1, &graphicsPipelineCI, nullptr, &pipeline),
                "Cannot create graphics pipeline!");

        // Destroy shader modules
        vkDestroyShaderModule(logicDevice, shaderStages[0].module, nullptr);
        vkDestroyShaderModule(logicDevice, shaderStages[1].module, nullptr);

        return;
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
