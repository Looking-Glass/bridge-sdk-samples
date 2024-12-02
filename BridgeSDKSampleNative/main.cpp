#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <GL/glext.h>
#include <string.h>
#include <bridge.h>
#include <bridge_utils.hpp>
#include <memory>
#include <codecvt>
#include <locale>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>

#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "renderer.h"
#include "shaderManager.h"
#include "gameWindow.h"

#include "MeshGenerator.h"

#ifdef _WIN32
#pragma optimize("", off)
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

// Global variables for mouse control
bool mousePressed = false;
double lastX = 0.0, lastY = 0.0;
float angleX = 0.0f, angleY = 0.0f;

float focus = -0.5f;
float depthiness = 1.0f;

Vector3 globalTarget = Vector3(0.0f, 0.0f, 0.0f); // Focus point
Vector3 globalEye = Vector3(0.0f, 0.0f, 5.0f);    // Camera position
Vector3 globalUp = Vector3(0.0f, 1.0f, 0.0f);     // Up direction
glm::quat cameraOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
float moveSpeed = 0.1f;                           // Movement speed

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) 
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) 
    {
        if (action == GLFW_PRESS) 
        {
            mousePressed = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        } 
        else if (action == GLFW_RELEASE) 
        {
            mousePressed = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) 
{
    if (mousePressed) 
    {
        float xoffset = float(xpos - lastX);
        float yoffset = float(ypos - lastY);
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.005f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        glm::vec3 up = glm::vec3(globalUp.x, globalUp.y, globalUp.z);
        glm::vec3 eye = glm::vec3(globalEye.x, globalEye.y, globalEye.z);
        glm::vec3 target = glm::vec3(globalTarget.x, globalTarget.y, globalTarget.z);

        // Create rotation quaternions for yaw (around global up) and pitch (around right vector)
        glm::quat yaw = glm::angleAxis(-xoffset, up); 
        glm::vec3 cameraRight = glm::normalize(glm::cross(up, (target - eye)));
        glm::quat pitch = glm::angleAxis(-yoffset, cameraRight);

        // Combine rotations and update orientation
        cameraOrientation = glm::normalize(yaw * cameraOrientation * pitch);
        cameraOrientation = glm::normalize(cameraOrientation);
        
        // Update camera position and target
        float distance = glm::length(eye - target);
        glm::vec3 forward = glm::rotate(cameraOrientation, glm::vec3(0.0f, 0.0f, -1.0f));
        eye = target - forward * distance;

        globalEye = Vector3(eye.x, eye.y, eye.z);
    }
}
// Scroll callback function
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Adjust the focus based on scroll direction
    focus += static_cast<float>(yoffset) * 0.075f; // Sensitivity
    depthiness += static_cast<float>(xoffset) * 0.075f; // Sensitivity
}

void processInput(GLFWwindow* window) 
{
    Vector3 forward = (globalTarget - globalEye).normalized(); // Forward direction
    Vector3 right = Vector3::cross(forward, globalUp).normalized(); // Right direction

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        globalTarget += globalUp * moveSpeed; // Move Up
        globalEye += globalUp * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        globalTarget -= globalUp * moveSpeed; // Move Down
        globalEye -= globalUp * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        globalTarget += forward * moveSpeed; // Move forward
        globalEye += forward * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        globalTarget -= forward * moveSpeed; // Move backward
        globalEye -= forward * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        globalTarget -= right * moveSpeed; // Pan left
        globalEye -= right * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        globalTarget += right * moveSpeed; // Pan right
        globalEye += right * moveSpeed;
    }

    // Update globalUp to remain perpendicular
    globalUp = Vector3::cross(right, forward).normalized();
}

int main(void)
{
    GameWindow gameWindow(800, 800, "Bridge SDK Native Sample");
    if (!gameWindow.initialize()) {
        return -1;
    }

    GLFWwindow* window = gameWindow.getWindow();
    const BridgeWindowData& bridgeData = gameWindow.getBridgeData();
    bool isBridgeDataInitialized = gameWindow.isBridgeInitialized();

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    Renderer renderer(800, 800, isBridgeDataInitialized, bridgeData);

    if (isBridgeDataInitialized) {
        renderer.initializeQuilt(bridgeData.vx, bridgeData.vy, bridgeData.view_width, bridgeData.view_height);
        //renderer.initializeQuilt(10, 10, 1920, 1080);
    }

    ShaderManager& shaderManager = ShaderManager::getInstance();
    shaderManager.setShaderPath("C:\\Repos\\LookingGlassBridge\\test_harnesses\\LookingGlassBridgeSDKExamples\\BridgeSDKSampleNative\\build\\Debug\\Assets");
    shaderManager.setShaderPath("Assets");

    Shader& shaderProgram = shaderManager.getShader("normal"); 

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearDepth(1.0f);
    glDepthRange(0.0f, 1.0f);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT, GL_FILL);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    Mesh cubeMesh = MeshGenerator::createCube();
    Mesh catMesh = Mesh::loadObj("C:\\Repos\\LookingGlassBridge\\test_harnesses\\LookingGlassBridgeSDKExamples\\BridgeSDKSampleNative\\build\\Debug\\Assets\\objs\\12221_Cat_v1_l3.obj",
                                   "C:\\Repos\\LookingGlassBridge\\test_harnesses\\LookingGlassBridgeSDKExamples\\BridgeSDKSampleNative\\build\\Debug\\Assets\\objs\\Cat_diffuse.jpg");
    
    std::vector<Mesh> sceneMeshes;
    sceneMeshes.push_back(std::move(cubeMesh));
    sceneMeshes.push_back(std::move(catMesh));

    renderer.setMeshes(std::move(sceneMeshes));

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        renderer.setWorldCamera(globalEye, globalTarget, globalUp);
        
        // Draw to primary head
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);  
        renderer.beginRender(false); // Render to the default framebuffer
        renderer.setViewport(0, 0, fbWidth, fbHeight);

        renderer.renderScene(shaderProgram);

        glfwSwapBuffers(window);

        if (isBridgeDataInitialized) {
            renderer.renderQuilt(shaderProgram, depthiness, focus);

            gameWindow.controller->DrawInteropQuiltTextureGL(bridgeData.wnd, renderer.getQuiltTexture(), PixelFormats::RGBA,
                                                renderer.quiltWidth, renderer.quiltHeight,
                                                renderer.quiltX, renderer.quiltY, bridgeData.displayaspect, 1.0f);
        }

        glfwPollEvents();
    }

    return 0;
}
