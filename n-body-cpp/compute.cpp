//
//  compute.cpp
//  n-body-cpp
//
//  Created by Jacob MacKenzie-Websdale on 03/12/2023.
//

#include "compute.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include "vulkan/vulkan.h"
//
//const char *shader =
//#include "../shaders/shader.spv"
//;

uint8_t ComputeShaderInterface::setup() {
    std::cout << "Setting up Vulkan" << std::endl;
    if (setupVulkan() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    setupPhysicalDevice();
    setupQueueFamilyIndex();
    
    std::cout << "Setting up Device" << std::endl;
    if (setupDevice() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    
    std::cout << "Setting up Queue" << std::endl;
    if (setupQueue() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    std::cout << "Setting up Shader" << std::endl;
    if (loadShader() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    std::cout << "Setting up Compute Pipeline" << std::endl;
    if (setupComputePipeline() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    std::cout << "Setting up Descriptor Sets" << std::endl;
    if (createDescriptorSet() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    std::cout << "Setting up Descriptor Pool" << std::endl;
    if (createDescriptorPool() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    std::cout << "Creating buffers" << std::endl;
    createAllBuffers();
    
    std::cout << "Re-allocating descriptor sets" << std::endl;
    allocateDescriptorSets();
    
    return EXIT_SUCCESS;
}

uint8_t ComputeShaderInterface::setupVulkan() {
    // Setup app info
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "N-body with C++";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
//    createInfo.enabledExtensionCount = 0;
//    createInfo.ppEnabledExtensionNames = nullptr;
    // Mac compatibility
    
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    std::vector<const char*> requiredExtensions;

    // Use built-ins
    for(uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtensions.emplace_back(glfwExtensions[i]);
    }

    // Use extra extensions
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    // Enable extensions
    createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();


    // If using validation layers, set them up here
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    
    // Create instance
    auto result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        // Handle errors
        std::cerr << "Failed to create Vulkan instance: " << result << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

void ComputeShaderInterface::setupPhysicalDevice() {
    physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Here, just pick the first device. In a real application, you would choose based on properties and features.
    physicalDevice = devices[0];
}

uint8_t ComputeShaderInterface::setupDevice() {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    float queuePriority = 1.0f;
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        std::cerr <<  "failed to create logical device!" << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

uint32_t ComputeShaderInterface::setupQueueFamilyIndex() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            std::cout << "Found queue family: " << i << std::endl;
            computeQueueFamilyIndex = i;
            break;
        }
    }
    if (computeQueueFamilyIndex == -1) {
        throw std::runtime_error("failed to find a queue family that supports compute operations");
    }
    
    return computeQueueFamilyIndex;
}

uint8_t ComputeShaderInterface::setupQueue() {
    vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);
    
    return EXIT_SUCCESS;
}

// See: https://stackoverflow.com/a/38559209
std::vector<char> ComputeShaderInterface::getShaderFromFile() {
    std::vector<char> vec;
    std::ifstream file;
    file.exceptions(
        std::ifstream::badbit
      | std::ifstream::failbit
      | std::ifstream::eofbit);
    
    //Need to use binary mode; otherwise CRLF line endings count as 2 for
    //`length` calculation but only 1 for `file.read` (on some platforms),
    //and we get undefined  behaviour when trying to read `length` characters.
    
    std::filesystem::path cwd = std::filesystem::current_path();
    
    // TODO : Make this inline or something...
    file.open("/Users/jacobmacweb/Documents/Projects/_personal/xcode/n-body-cpp/shaders/shader.spv", std::ifstream::in | std::ifstream::binary);
    file.seekg(0, std::ios::end);
    std::streampos length(file.tellg());
    if (length) {
        file.seekg(0, std::ios::beg);
        vec.resize(static_cast<std::size_t>(length));
        file.read(&vec.front(), static_cast<std::size_t>(length));
    }
    
    return vec;
}

uint8_t ComputeShaderInterface::loadShader() {
    std::vector<char> shaderCode = getShaderFromFile();
    
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderCode.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
    
    if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr <<  "failed to create shader module!" << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

uint8_t ComputeShaderInterface::setupComputePipeline() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cerr <<  "failed to create pipeline layout!" << std::endl;
        return EXIT_FAILURE;
    }

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineCreateInfo.stage.module = shaderModule;
    pipelineCreateInfo.stage.pName = "main";
    pipelineCreateInfo.layout = pipelineLayout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        std::cerr <<  "failed to create compute pipeline!" << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

// Descriptor set
uint8_t ComputeShaderInterface::createDescriptorSet() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding inputLayoutBinding{};
    inputLayoutBinding.binding = 2;
    inputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputLayoutBinding.descriptorCount = 1;
    inputLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    inputLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding outputLayoutBinding{};
    outputLayoutBinding.binding = 1;
    outputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputLayoutBinding.descriptorCount = 1;
    outputLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    outputLayoutBinding.pImmutableSamplers = nullptr; // Optional

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {uboLayoutBinding, inputLayoutBinding, outputLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "failed to create descriptor set layout!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
    // You'll also need to create a descriptor pool and allocate descriptor sets from it, then update the sets with the buffers
}

uint8_t ComputeShaderInterface::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

//    VkDescriptorPoolCreateInfo poolInfo{};
//    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//    poolInfo.poolSizeCount = 1;
//    poolInfo.pPoolSizes = &poolSize;
//    poolInfo.maxSets = 1;
    
    // This sizes the descriptor pool to match the demands of the descriptor sets
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }  // Two storage buffers
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;  // Number of descriptor sets

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    auto result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
    if (result != VK_SUCCESS) {
        std::cerr << "failed to allocate descriptor set: " << result << std::endl;
        return EXIT_FAILURE;
    }

    // Update descriptor sets with the uniform, input, and output buffers
    return EXIT_SUCCESS;
}


// Buffers
void ComputeShaderInterface::createUniformBuffer() {
    genericCreateBuffer(device, physicalDevice, sizeof(UniformBlock), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
    
    uniformBufferInfo.buffer = uniformBuffer; // Your UBO VkBuffer
    uniformBufferInfo.offset = 0; // Start from the beginning of the buffer
    uniformBufferInfo.range = sizeof(UniformBlock); // The size of your UBO data

    
}

void ComputeShaderInterface::createInputBuffer() {
    genericCreateBuffer(device, physicalDevice, sizeof(InputData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, inputBuffer, inputBufferMemory);
    
    inputBufferInfo.buffer = inputBuffer; // Your input data VkBuffer
    inputBufferInfo.offset = 0; // Start from the beginning of the buffer
    inputBufferInfo.range = sizeof(InputData); // Size of the input data

}

void ComputeShaderInterface::createOutputBuffer() {
    genericCreateBuffer(device, physicalDevice, sizeof(OutputData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outputBuffer, outputBufferMemory);
    
    outputBufferInfo.buffer = outputBuffer; // Your output data VkBuffer
    outputBufferInfo.offset = 0; // Start from the beginning of the buffer
    outputBufferInfo.range = sizeof(OutputData); // Size of the output data

    
}

void ComputeShaderInterface::createAllBuffers() {
    createUniformBuffer();
    createInputBuffer();
    createOutputBuffer();
}

void ComputeShaderInterface::allocateDescriptorSets() {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &outputBufferInfo;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &inputBufferInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

}

void ComputeShaderInterface::mapMemory(void** output) {
    vkMapMemory(device, inputBufferMemory, 0, sizeof(InputData), 0, &inputData);
    vkMapMemory(device, outputBufferMemory, 0, sizeof(OutputData), 0, output);
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(UniformBlock), 0, &uniformData);
}

void ComputeShaderInterface::copyToBuffer(std::array<Particle, MAX_PARTICLE_COUNT> particles, float dt) {
    InputData ip = {
        .input_data = particles
    };
    
    memcpy(inputData, &ip, (size_t)sizeof(InputData));

    // Similarly for the uniform buffer
    UniformBlock ubo = {
        .u_particle_count = MAX_PARTICLE_COUNT,
        .u_dt =  dt
    };
    
    memcpy(uniformData, &ubo, sizeof(UniformBlock));
}

void ComputeShaderInterface::dispatchShader() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = computeQueueFamilyIndex; // Use the correct queue family index
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    VkCommandBuffer commandBuffer;
    
    // Barrier
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(commandBuffer, 1, 1, 1);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(computeQueue);
}

void ComputeShaderInterface::retrieveResult(void** data) {
    vkMapMemory(device, outputBufferMemory, 0, sizeof(OutputData), 0, data);
}

void ComputeShaderInterface::retrieveResultCleanup() {
    vkUnmapMemory(device, outputBufferMemory);
}

void ComputeShaderInterface::genericCreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    std::cout << "memtypeindex: " << allocInfo.memoryTypeIndex << std::endl;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    auto result = vkBindBufferMemory(device, buffer, bufferMemory, 0);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to bind memory : " << result << std::endl;
    }
    
    std::cout << "Bound memory" << std::endl;
}

uint32_t ComputeShaderInterface::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void ComputeShaderInterface::cleanup() {
    vkDestroyShaderModule(device, shaderModule, nullptr);
    vkDestroyPipeline(device, computePipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    // Destroy buffers, free memory, etc.
    vkUnmapMemory(device, inputBufferMemory);
    vkUnmapMemory(device, uniformBufferMemory);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}
