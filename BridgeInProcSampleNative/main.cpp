#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <GL/glext.h>
#include <string.h>
#include <bridge.h>
#include <bridge_utils.hpp>
#include <LKGCamera.h>
#include <memory>
#include <codecvt>
#include <locale>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <ogl.h>

#ifdef _WIN32
#pragma optimize("", off)
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

const char* vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "out vec3 vertexColor;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "    vertexColor = color;\n"
    "}\n";

const char* fragmentShaderSource = 
    "#version 330 core\n"
    "in vec3 vertexColor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(vertexColor, 1.0);\n"
    "}";

const float vertices[] =
{
    // Positions           // Colors
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  // Front face
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  // Back face
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  // Left face
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,

     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  // Right face
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  // Bottom face
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  // Top face
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f
};

const unsigned int indices[] =
{
    0, 1, 2, 2, 3, 0,        // Front face
    4, 5, 6, 6, 7, 4,        // Back face
    8, 9, 10, 10, 11, 8,     // Left face
    12, 13, 14, 14, 15, 12,  // Right face
    16, 17, 18, 18, 19, 16,  // Bottom face
    20, 21, 22, 22, 23, 20   // Top face
};

// Global variables for mouse control
bool mousePressed = false;
double lastX = 0.0, lastY = 0.0;
float angleX = 0.0f, angleY = 0.0f;

float focus = 0.0f;
float depthiness = 1.0f;

void drawScene(GLuint shaderProgram, GLuint vao, LKGCamera& camera, float normalizedView = 0.5f, bool invert = false, float depthiness = 0.0f, float focus = 0.0f)
{
    ogl::glBindVertexArray(vao);
    ogl::glUseProgram(shaderProgram);

    // Compute view and projection matrices using LKGCamera
    Matrix4 viewMatrix;
    Matrix4 projectionMatrix;
    camera.computeViewProjectionMatrices(normalizedView, invert, depthiness, focus, viewMatrix, projectionMatrix);

    // Compute the model matrix (e.g., rotating cube)
    Matrix4 modelMatrix = camera.getModelMatrix(angleX, angleY);

    // Set uniforms
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, modelMatrix.m);
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, viewMatrix.m);
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projectionMatrix.m);

    // Draw the object
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

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
        angleX += yoffset * 0.005f; // Sensitivity
        angleY += xoffset * 0.005f; // Sensitivity
    }
}

// Scroll callback function
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Adjust the focus based on scroll direction
    focus += static_cast<float>(yoffset) * 0.075f; // Sensitivity
    depthiness += static_cast<float>(xoffset) * 0.075f; // Sensitivity
}

int main(void)
{
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(800, 800, "Bridge SDK Native Sample -- No Device Connected!", nullptr, nullptr);
    
    if (!window) 
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    ogl::loadOpenGLFunctions();

    // Create the controller
    std::unique_ptr<Controller> controller = std::make_unique<Controller>();

#ifdef _WIN32
    if (!controller->Initialize(L"BridgeSDKSampleNative"))
#else
    if (!controller->Initialize("BridgeSDKSampleNative"))
#endif
    {
        controller = nullptr;
    }

    WINDOW_HANDLE wnd = 0;
    std::vector<DisplayInfo> displays;

    if (controller)
    {
        // Get display information list
        displays = controller->GetDisplayInfoList();

        // Print all display names
        for (const auto& displayInfo : displays)
        {
            std::wcout << displayInfo.name << std::endl;
        }

        // For now we will use the first looking glass display
        if (!displays.empty() && controller->InstanceWindowGL(&wnd, displays[0].display_id))
        {
            // Successfully created the window handle
        }
        else
        {
            wnd = 0;
        }
    }
    
    BridgeWindowData bridgeData = controller ? controller->GetWindowData(wnd) : BridgeWindowData();
    bool isBridgeDataInitialized = (bridgeData.wnd != 0);

    // Update window size and title if BridgeData is initialized
    if (isBridgeDataInitialized)
    {
        // Find display info for the window title based on display index
        auto displayInfoIt = std::find_if(
            displays.begin(),
            displays.end(),
            [&bridgeData](const DisplayInfo& info)
            {
                return info.display_id == bridgeData.display_index;
            }
        );

        if (displayInfoIt != displays.end())
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::string window_title = "Bridge SDK Native Sample -- " +
                converter.to_bytes(displayInfoIt->name) +
                " : " + converter.to_bytes(displayInfoIt->serial);
            glfwSetWindowTitle(window, window_title.c_str());
        }

        // set 2d window to be half the size of the looking glass display we are outputting to 
        int window_width  = (int)bridgeData.output_width / 2;
        int window_height = (int)bridgeData.output_height / 2;

        glfwSetWindowSize(window, window_width, window_height);
    }

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    float size = 10.0f;
    Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

    // fov should not be viewcone, they are separate!
    // float fov = isBridgeDataInitialized ? bridgeData.viewcone : 45.0f;
    float fov = 14.0f;
    float viewcone = isBridgeDataInitialized ? bridgeData.viewcone : 40.0f;
    float aspect = isBridgeDataInitialized ? bridgeData.displayaspect : 1.0f;
    float nearPlane = 0.001f;
    float farPlane = 100.0f;

    // LKGCamera camera = LKGCamera(position, target, up, fov, aspect, nearPlane, farPlane);
    LKGCamera camera = LKGCamera(size, target, up, fov, viewcone, aspect, nearPlane, farPlane);

    // Initialize OpenGL textures and framebuffers
    GLuint render_texture = 0;
    GLuint render_fbo = 0;
    GLuint depth_buffer = 0;

    if (isBridgeDataInitialized)
    {
        // Initialize OpenGL textures and framebuffers using bridgeData's quilt dimensions
        glGenTextures(1, &render_texture);
        glBindTexture(GL_TEXTURE_2D, render_texture);
        ogl::glTexImage2D(GL_TEXTURE_2D, 
                        0, 
                        GL_RGBA, 
                        bridgeData.quilt_width, 
                        bridgeData.quilt_height, 
                        0, 
                        GL_RGBA,
                        GL_UNSIGNED_BYTE, 
                        nullptr); 

        // Generate and bind the renderbuffer for depth
        ogl::glGenRenderbuffers(1, &depth_buffer);
        ogl::glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer); 

        // Create a depth buffer
        ogl::glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, bridgeData.quilt_width, bridgeData.quilt_height);

        // Generate the framebuffer
        ogl::glGenFramebuffers(1, &render_fbo);
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);

        // Attach the texture to the framebuffer as a color attachment
        ogl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_texture, 0);

        // Attach the renderbuffer as a depth attachment
        ogl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

        ogl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ogl::glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    GLuint shaderProgram = ogl::createProgram(vertexShaderSource, fragmentShaderSource);

    GLuint vao, vbo, ebo;
    ogl::glGenVertexArrays(1, &vao);
    ogl::glBindVertexArray(vao);

    ogl::glGenBuffers(1, &vbo);
    ogl::glBindBuffer(GL_ARRAY_BUFFER, vbo);
    ogl::glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    ogl::glGenBuffers(1, &ebo);
    ogl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    ogl::glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    ogl::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    ogl::glEnableVertexAttribArray(0);

    ogl::glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    ogl::glEnableVertexAttribArray(1);

    ogl::glUseProgram(shaderProgram);
    ogl::glBindVertexArray(vao);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearDepth(1.0f);
    glDepthRange(0.0f, 1.0f);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT, GL_FILL);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    int totalViews = bridgeData.vx * bridgeData.vy;

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        // Draw to primary head
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);  
        glViewport(0, 0, fbWidth, fbHeight);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawScene(shaderProgram, vao, camera);
        
        glfwSwapBuffers(window);

        if (isBridgeDataInitialized)
        {
            // Draw the quilt views for the hologram
            ogl::glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (int y = 0; y < bridgeData.vy; y++)
            {
                for (int x = 0; x < bridgeData.vx; x++)
                {
                    int invertedY = bridgeData.vy - 1 - y;
                    glViewport(x * bridgeData.view_width, invertedY * bridgeData.view_height, bridgeData.view_width, bridgeData.view_height);

                    int viewIndex = y * bridgeData.vx + x;
                    float normalizedView = static_cast<float>(viewIndex) / static_cast<float>(totalViews - 1);

                    drawScene(shaderProgram, vao, camera, normalizedView, true, depthiness, focus);
                }
            }

            controller->DrawInteropQuiltTextureGL(bridgeData.wnd, render_texture, PixelFormats::RGBA,
                                                bridgeData.quilt_width, bridgeData.quilt_height,
                                                bridgeData.vx, bridgeData.vy, bridgeData.displayaspect, 1.0f);
        }

        glfwPollEvents();
    }

    // Cleanup
    if (controller)
    {
        controller->Uninitialize();
    }

    // Delete OpenGL resources
    ogl::glDeleteVertexArrays(1, &vao);
    ogl::glDeleteBuffers(1, &vbo);
    ogl::glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &render_texture);
    ogl::glDeleteRenderbuffers(1, &depth_buffer);
    ogl::glDeleteFramebuffers(1, &render_fbo);
    ogl::glDeleteProgram(shaderProgram);

    glfwTerminate();

    return 0;
}
