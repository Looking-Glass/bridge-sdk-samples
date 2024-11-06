#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <GL/glext.h>
#include <string.h>
#include <bridge.h>
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

// Global variables for mouse control
bool mousePressed = false;
double lastX = 0.0, lastY = 0.0;
float angleX = 0.0f, angleY = 0.0f;

void drawScene(GLuint shaderProgram, GLuint vao, LKGCamera& camera, float normalizedView = 0.5f, bool invert = false, float depthiness = 0.0f, float focus = 0.0f)
{
    ogl::glBindVertexArray(vao);
    ogl::glUseProgram(shaderProgram);

    // Compute view and projection matrices using LKGCamera
    Matrix4 viewMatrix;
    Matrix4 projectionMatrix;
    camera.computeViewProjectionMatrices(normalizedView, invert, depthiness, focus, viewMatrix, projectionMatrix);

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

    window = glfwCreateWindow(800, 800, "Bridge InProc SDK Native Interactive Sample -- No Device Connected!", nullptr, nullptr);
    
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
    if (!controller->Initialize(L"BridgeInProcSampleNative"))
    #else
    if (!controller->Initialize("BridgeInProcSampleNative"))
    #endif
    {
        controller = nullptr;
    }

    // Create BridgeData
    WINDOW_HANDLE wnd = 0;
    if (controller)
    {
        // Get displays
        int display_count = 0;
        controller->GetDisplays(&display_count, nullptr);
        std::vector<unsigned long> display_ids(display_count);
        controller->GetDisplays(&display_count, display_ids.data());

        if (!display_ids.empty())
        {
            unsigned long first_display_id = display_ids.front();
            if (controller->InstanceOffscreenWindowGL(&wnd, first_display_id))
            {
                // Window handle created successfully
            }
            else
            {
                wnd = 0;
            }
        }
    }

    BridgeData bridgeData = BridgeData::Create(*controller, wnd);
    bool isBridgeDataInitialized = (bridgeData.wnd != 0);

    // Update window size and title
    if (isBridgeDataInitialized)
    {
        glfwSetWindowSize(window, bridgeData.output_width, bridgeData.output_height);

        // Set window title using display info
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        auto it = std::find_if(bridgeData.display_infos.begin(), bridgeData.display_infos.end(),
                            [&bridgeData](const DisplayInfo& info) {
                                return info.display_id == bridgeData.display_index;
                            });

        if (it != bridgeData.display_infos.end())
        {
            const DisplayInfo& displayInfo = *it;
            std::string window_title = "Bridge InProc SDK Native Interactive Sample -- " +
                                    converter.to_bytes(displayInfo.name) +
                                    " : " + converter.to_bytes(displayInfo.serial);
            glfwSetWindowTitle(window, window_title.c_str());
        }

        // Move the window to the correct monitor based on bridgeData.window_position
        int monitorCount;
        GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
        GLFWmonitor* targetMonitor = nullptr;

        // Find the monitor that matches the position in bridgeData.window_position
        for (int i = 0; i < monitorCount; i++)
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
            // Get the video mode of the target monitor
            const GLFWvidmode* mode = glfwGetVideoMode(targetMonitor);

            // Set the window to fullscreen on the target monitor
            glfwSetWindowMonitor(window, targetMonitor, 0, 0, bridgeData.output_width, bridgeData.output_height, GLFW_DONT_CARE);
        }
        else
        {
            // If no matching monitor is found, position the window at the specified coordinates
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

    GLuint shaderProgram    = ogl::createProgram(vertexShaderSource, fragmentShaderSource);
    GLuint shaderProgramTex = ogl::createProgram(vertexShaderSourceTex, fragmentShaderSourceTex);

    float vertices[] = 
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

    unsigned int indices[] = 
    {
        0, 1, 2, 2, 3, 0,        // Front face
        4, 5, 6, 6, 7, 4,        // Back face
        8, 9, 10, 10, 11, 8,     // Left face
        12, 13, 14, 14, 15, 12,  // Right face
        16, 17, 18, 18, 19, 16,  // Bottom face
        20, 21, 22, 22, 23, 20   // Top face
    };

    Vector3 position = Vector3(0.0f, 0.0f, 5.0f);
    Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

    float fov = isBridgeDataInitialized ? bridgeData.viewcone : 45.0f;
    float aspect = isBridgeDataInitialized ? bridgeData.displayaspect : 1.0f;
    float nearPlane = 0.001f;
    float farPlane = 100.0f;

    LKGCamera camera = LKGCamera(position, target, up, fov, aspect, nearPlane, farPlane);

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

     // Vertex data for a full-screen quad
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 0.0f
    };

    unsigned int quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

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

    int totalViews = bridgeData.vx * bridgeData.vy;
    float depthiness = 1.0f;
    float focus = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
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
                    int invertedY = bridgeData.vy - 1 - y;
                    glViewport(x * bridgeData.view_width, invertedY * bridgeData.view_height, bridgeData.view_width, bridgeData.view_height);

                    int viewIndex = y * bridgeData.vx + x;
                    float normalizedView = static_cast<float>(viewIndex) / static_cast<float>(totalViews - 1);

                    drawScene(shaderProgram, vaoCube, camera, normalizedView, true, depthiness, focus);
                }
            }

            controller->DrawInteropQuiltTextureGL(bridgeData.wnd, render_texture, PixelFormats::RGBA,
                                                bridgeData.quilt_width, bridgeData.quilt_height,
                                                bridgeData.vx, bridgeData.vy, bridgeData.displayaspect, 1.0f);
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
            unsigned long      hologram_width   = 0;
            unsigned long      hologram_height  = 0;
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
