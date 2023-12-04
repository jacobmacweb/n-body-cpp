//
//  compute.hpp
//  n-body-cpp
//
//  Created by Jacob MacKenzie-Websdale on 03/12/2023.
//

#ifndef compute_hpp
#define compute_hpp

#include <stdio.h>
#include "vulkan/vulkan.h"
#include <array>

class ComputeShaderInterface {
public:
    void cleanup();
    uint8_t setupVulkan();
    uint8_t setupDevice();
    uint8_t setupShaderModule();
    uint8_t setupComputePipeline();
    void setDescriptorLayout();
    void setupDescriptorPool();
    void allocateDescriptorSets();
    uint8_t setupCommandPool();
    uint8_t setupCommandBuffer();
private:
    VkInstance instance;
    VkDevice device;
    VkShaderModule shaderModule;
    VkPipeline computePipeline;
    VkDescriptorSet descriptorSet;
    std::array<VkWriteDescriptorSet, 3> descriptorWrites;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
};



#endif /* compute_hpp */
