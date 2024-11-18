#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <GL/glext.h>
#include <string.h>
#include <bridge.h>
#include <bridge_utils.hpp>
#include <LKGCamera.hpp>
#include <memory>
#include <codecvt>
#include <locale>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <ogl.h>
#include <chrono>
#include <numeric>


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

const char* vertexShaderSourceTex = 
    "#version 330 core\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec2 texCoord;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(position, 1.0);\n"
    "    TexCoord = texCoord;\n"
    "}\n";

const char* fragmentShaderSourceTex = 
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "    FragColor = texture(texture1, TexCoord);\n"
    "}\n";

// Vertex data for a cube
const float vertices[] =
{
    // Positions           // Colors
    -1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,  // Front face
     1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
     1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
    -1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 0.0f,

    -1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 0.0f,  // Back face
     1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
    -1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f,

    -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 1.0f,  // Left face
    -1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
    -1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
    -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 1.0f,

     1.0f, -1.0f, -1.0f,  1.0f, 1.0f, 0.0f,  // Right face
     1.0f, -1.0f,  1.0f,  1.0f, 1.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, -1.0f,  1.0f, 1.0f, 0.0f,

    -1.0f, -1.0f, -1.0f,  0.0f, 1.0f, 1.0f,  // Bottom face
     1.0f, -1.0f, -1.0f,  0.0f, 1.0f, 1.0f,
     1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 1.0f,
    -1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 1.0f,

    -1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 1.0f,  // Top face
     1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 1.0f,
    -1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 1.0f
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

// Vertex data for a full-screen quad
const float quadVertices[] = {
    // positions        // texture Coords
    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
     1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
     1.0f,  1.0f, 0.0f,  1.0f, 0.0f
};

const unsigned int quadIndices[] = {
    0, 1, 2,
    2, 3, 0
};

// Global variables for mouse control
bool mousePressed = false;
double lastX = 0.0, lastY = 0.0;
float angleX = 0.0f, angleY = 0.0f;

float focus = -0.5f;
float offset_mult = 1.0f;

void drawScene(GLuint shaderProgram, GLuint vao, LKGCamera& camera, float normalizedView = 0.5f, bool invert = false, float offset_mult = 0.0f, float focus = 0.0f)
{
    ogl::glBindVertexArray(vao);
    ogl::glUseProgram(shaderProgram);

    // Compute view and projection matrices using LKGCamera
    Matrix4 viewMatrix;
    Matrix4 projectionMatrix;
    camera.computeViewProjectionMatrices(normalizedView, invert, offset_mult, focus, viewMatrix, projectionMatrix);

    Matrix4 modelMatrix = camera.getModelMatrix(angleX, angleY);

    // Set uniforms
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, modelMatrix.m);
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, viewMatrix.m);
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projectionMatrix.m);

    // Draw the object
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

void drawQuad(GLuint shaderProgram, GLuint vao, GLuint texture)
{
    ogl::glActiveTexture(GL_TEXTURE0); 
    glBindTexture(GL_TEXTURE_2D, texture);

    ogl::glUseProgram(shaderProgram);

    GLint texLocation = ogl::glGetUniformLocation(shaderProgram, "texture1");
    ogl::glUniform1i(texLocation, 0);

    ogl::glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    ogl::glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    ogl::glUseProgram(0);
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
    offset_mult += static_cast<float>(xoffset) * 0.075f; // Sensitivity
}

int main(void)
{
    GLFWwindow* window;
    GLuint render_texture = 0;
    GLuint render_fbo = 0;
    GLuint depth_buffer = 0;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);         // Remove window decorations
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);         // Make the window non-resizable

    window = glfwCreateWindow(800, 800, "Bridge SDK Native Interactive Sample -- No Device Connected!", nullptr, nullptr);
    
    if (!window) 
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    ogl::loadOpenGLFunctions();
    
    // Create the bridge controller
    std::unique_ptr<Controller> controller = std::make_unique<Controller>();

    #ifdef _WIN32
    if (!controller->Initialize(L"BridgeSDKSampleNative"))
    #else
    if (!controller->Initialize("BridgeSDKSampleNative"))
    #endif
    {
        controller = nullptr;
        std::wcout << "Failed to initialize bridge. Bridge may be missing, or the version may be too old" << std::endl;
    }

    // Create BridgeData
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
        if (!displays.empty() && controller->InstanceOffscreenWindowGL(&wnd, displays[0].display_id))
        {
            // Successfully created the window handle
        }
        else
        {
            wnd = 0;
            std::wcout << "Failed to initialize bridge window. do you have any displays connected?" << std::endl;
        }
    }

    BridgeWindowData bridgeData = controller ? controller->GetWindowData(wnd) : BridgeWindowData();
    bool isBridgeDataInitialized = (bridgeData.wnd != 0);

    std::string window_title = "";

    // Update window size and title if BridgeData is initialized
    if (isBridgeDataInitialized)
    {
        // Set focus based on aspect ratio
        // Landscape displays need around -0.5f
        // Widescreen displays need around -2f
        focus = bridgeData.displayaspect > 1.0 ? -0.5f : -2.0f;
        
        // multiplies the depthiness of the 3D output
        // with a value of 1.0 objects should appear physically accurate 
        // when on the focal plane
        offset_mult = 1.0f; 

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
            window_title = "Bridge SDK Native Interactive Sample -- " +
                converter.to_bytes(displayInfoIt->name) +
                " : " + converter.to_bytes(displayInfoIt->serial);
            glfwSetWindowTitle(window, window_title.c_str());
        }

        // Optionally find and set the correct monitor based on window position for exclusive full screen
        bool useExclusiveFullScreen = false;
        if (useExclusiveFullScreen)
        {
            int monitorCount;
            GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
            GLFWmonitor* targetMonitor = nullptr;

            for (int i = 0; i < monitorCount; ++i)
            {
                int xpos = 0, ypos = 0;
                glfwGetMonitorPos(monitors[i], &xpos, &ypos);

                if (xpos == bridgeData.window_position.x && ypos == bridgeData.window_position.y)
                {
                    targetMonitor = monitors[i];
                    break;
                }
            }

            if (targetMonitor)
            {
                const GLFWvidmode* mode = glfwGetVideoMode(targetMonitor);
                glfwSetWindowMonitor(window, targetMonitor, 0, 0, bridgeData.output_width, bridgeData.output_height, GLFW_DONT_CARE);
            }
            else
            {
                glfwSetWindowSize(window, bridgeData.output_width, bridgeData.output_height);
                glfwSetWindowPos(window, bridgeData.window_position.x, bridgeData.window_position.y);
            }
        }
        else
        {
            glfwSetWindowSize(window, bridgeData.output_width, bridgeData.output_height);
            glfwSetWindowPos(window, bridgeData.window_position.x, bridgeData.window_position.y);
        }

        // Initialize OpenGL textures and framebuffers using bridgeData's quilt dimensions
        glGenTextures(1, &render_texture);
        glBindTexture(GL_TEXTURE_2D, render_texture);

        // Set texture parameters
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
    else
    {
        // Handle case when bridge is not initialized
        float window_width = 800;
        float window_height = 800;
        glfwSetWindowSize(window, window_width, window_height);
    }

    // Register mouse callback functions
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    GLuint shaderProgram    = ogl::createProgram(vertexShaderSource, fragmentShaderSource);
    GLuint shaderProgramTex = ogl::createProgram(vertexShaderSourceTex, fragmentShaderSourceTex);

    GLuint vaoCube, vboCube, eboCube;
    ogl::glGenVertexArrays(1, &vaoCube);
    ogl::glBindVertexArray(vaoCube);

    ogl::glGenBuffers(1, &vboCube);
    ogl::glBindBuffer(GL_ARRAY_BUFFER, vboCube);
    ogl::glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    ogl::glGenBuffers(1, &eboCube);
    ogl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboCube);
    ogl::glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    ogl::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    ogl::glEnableVertexAttribArray(0);

    ogl::glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    ogl::glEnableVertexAttribArray(1);

    ogl::glBindVertexArray(0);

    GLuint vaoQuad, vboQuad, eboQuad;
    ogl::glGenVertexArrays(1, &vaoQuad);
    ogl::glBindVertexArray(vaoQuad);

    ogl::glGenBuffers(1, &vboQuad);
    ogl::glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
    ogl::glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    ogl::glGenBuffers(1, &eboQuad);
    ogl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboQuad);
    ogl::glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    ogl::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    ogl::glEnableVertexAttribArray(0);

    ogl::glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    ogl::glEnableVertexAttribArray(1);

    ogl::glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearDepth(1.0f);
    glDepthRange(0.0f, 1.0f);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    float size = 10.0f;
    Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

    float fov = 14.0f;
    float viewcone = isBridgeDataInitialized ? bridgeData.viewcone : 40.0f;
    float aspect = isBridgeDataInitialized ? bridgeData.displayaspect : 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    LKGCamera camera = LKGCamera(size, target, up, fov, viewcone, aspect, nearPlane, farPlane);
    
    int totalViews = bridgeData.vx * bridgeData.vy;

    std::vector<float> frameTimes;
    const int maxSamples = 100; // Number of samples to calculate the average
    auto lastTime = std::chrono::high_resolution_clock::now();

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Record frame time
        if (frameTimes.size() < maxSamples)
        {
            frameTimes.push_back(1.0f / deltaTime); // FPS = 1 / deltaTime
        }
        else
        {
            frameTimes.erase(frameTimes.begin());
            frameTimes.push_back(1.0f / deltaTime);
        }

        glfwMakeContextCurrent(window);

        if (controller && bridgeData.wnd != 0)
        {
            // Draw the quilt views for the hologram
            ogl::glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (int y = 0; y < bridgeData.vy; y++)
            {
                for (int x = 0; x < bridgeData.vx; x++)
                {
                    int viewPositionX = x * bridgeData.view_width;

                    int invertedY = bridgeData.vy - 1 - y;
                    int viewPositionY = invertedY * bridgeData.view_height;

                    int viewIndex = y * bridgeData.vx + x;
                    float normalizedView = static_cast<float>(viewIndex) / static_cast<float>(totalViews - 1);

                    glViewport(viewPositionX, viewPositionY, bridgeData.view_width, bridgeData.view_height);
                    drawScene(shaderProgram, vaoCube, camera, normalizedView, true, offset_mult, focus);
                }
            }

            controller->DrawInteropQuiltTextureGL(bridgeData.wnd, render_texture, PixelFormats::RGBA,
                bridgeData.quilt_width, bridgeData.quilt_height,
                bridgeData.vx, bridgeData.vy, bridgeData.displayaspect, 1.0f);

            if (controller->IsDisplayDisconnected(displays[0].serial))
            {
                int sizeX = 0;
                int sizeY = 0;
                glfwGetWindowSize(window, &sizeX, &sizeY);

                if (sizeX != 800 && sizeY != 800)
                {
                    glfwSetWindowSize(window, 800, 800);
                }
            }
        }

        glfwMakeContextCurrent(window);

        // mlc: draw to primary head
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // mlc: retina immune viewport
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (controller && bridgeData.wnd != 0)
        {
            // Draw hologram
            unsigned long long hologram_texture = 0;
            unsigned long      hologram_width = 0;
            unsigned long      hologram_height = 0;
            PixelFormats       hologram_format = PixelFormats::NoFormat;

            controller->GetOffscreenWindowTextureGL(bridgeData.wnd, &hologram_texture, &hologram_format, &hologram_width, &hologram_height);
            drawQuad(shaderProgramTex, vaoQuad, (GLuint)hologram_texture);
        }
        else
        {
            // No device connected -- draw single view
            drawScene(shaderProgram, vaoCube, camera);
        }

        glfwSwapBuffers(window);

        glfwPollEvents();

        float averageFPS = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
        std::stringstream ss;
        ss << window_title.c_str();
        ss << " ";
        ss << averageFPS;

        glfwSetWindowTitle(window, ss.str().c_str());
    }

    if (controller)
    {
        controller->Uninitialize();
    }

    ogl::glDeleteVertexArrays(1, &vaoCube);
    ogl::glDeleteBuffers(1, &vboCube);
    ogl::glDeleteBuffers(1, &eboCube);
    ogl::glDeleteVertexArrays(1, &vaoQuad);
    ogl::glDeleteBuffers(1, &vboQuad);
    ogl::glDeleteBuffers(1, &eboQuad);
    glDeleteTextures(1, &render_texture);
    ogl::glDeleteRenderbuffers(1, &depth_buffer);
    ogl::glDeleteFramebuffers(1, &render_fbo);
    ogl::glDeleteProgram(shaderProgram);
    ogl::glDeleteProgram(shaderProgramTex);

    glfwTerminate();

    return 0;
}
