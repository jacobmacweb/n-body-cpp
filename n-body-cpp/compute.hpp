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

const uint64_t MAX_PARTICLE_COUNT  = /* 100 */ 100;

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float mass;
};

struct InputData {
    std::array<Particle, MAX_PARTICLE_COUNT> input_data;
};

struct OutputData {
    std::array<Particle, MAX_PARTICLE_COUNT> next;
};

struct UniformBlock {
    float u_dt;
    
    // Future proofing for variably sized particle counts
    int u_particle_count;
};

class ComputeShaderInterface {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t computeQueueFamilyIndex = -1;
    VkQueue computeQueue;
    VkShaderModule shaderModule;
    
    
    VkPipelineLayout pipelineLayout;
    VkPipeline computePipeline;
    
    // buffer
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    VkBuffer inputBuffer;
    VkDeviceMemory inputBufferMemory;
    VkBuffer outputBuffer;
    VkDeviceMemory outputBufferMemory;
    
    // mappings
    void* inputData;
    void* uniformData;
    
    // desciptor set
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;
    
    // data
    VkCommandPool commandPool;
    
    
    VkDescriptorBufferInfo uniformBufferInfo;
    VkDescriptorBufferInfo inputBufferInfo;
    VkDescriptorBufferInfo outputBufferInfo;
    
public:
    uint8_t setup();
    
    // This is described in the order of execution.
    uint8_t setupVulkan();
    void setupPhysicalDevice();
    uint8_t setupDevice();
    uint32_t setupQueueFamilyIndex();
    uint8_t setupQueue();
    uint8_t loadShader();
    uint8_t setupComputePipeline();
    
    // Descriptor sets
    uint8_t createDescriptorSet();
    uint8_t createDescriptorPool();
    
    void createUniformBuffer();
    void createInputBuffer();
    void createOutputBuffer();
    
    void createAllBuffers();
    
    void allocateDescriptorSets();
    
    void mapMemory(void** output);
    
    // Data phase
    void copyToBuffer(std::array<Particle, MAX_PARTICLE_COUNT> particles, float dt);
    void dispatchShader();
    
    void retrieveResult(void** data);
    void retrieveResultCleanup();
    
    // Clean-up
    void cleanup();
private:
    std::vector<char> getShaderFromFile();
    
    // generic
    void genericCreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
};




#endif /* compute_hpp */
