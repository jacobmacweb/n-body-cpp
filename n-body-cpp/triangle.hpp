//
//  triangle.hpp
//  n-body-cpp
//
//  Created by Jacob MacKenzie-Websdale on 03/12/2023.
//

#ifndef triangle_hpp
#define triangle_hpp

#include <stdio.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class HelloTriangleApplication {
public:
    void run();

private:
    GLFWwindow* window;
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
};

#endif /* triangle_hpp */
