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

#ifdef _WIN32
#pragma optimize("", off)
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

using namespace std;

typedef void (*PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels);

#ifdef __APPLE__
typedef void (*PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void (*PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (*PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (*PFNGLDELETESHADERPROC)(GLuint);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void (*PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint program);
#endif

namespace ogl 
{
    PFNGLGENBUFFERSPROC              glGenBuffers = nullptr;
    PFNGLBINDBUFFERPROC              glBindBuffer = nullptr;
    PFNGLBUFFERDATAPROC              glBufferData = nullptr;
    PFNGLGENVERTEXARRAYSPROC         glGenVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC         glBindVertexArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
    PFNGLCREATESHADERPROC            glCreateShader = nullptr;
    PFNGLSHADERSOURCEPROC            glShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC           glCompileShader = nullptr;
    PFNGLCREATEPROGRAMPROC           glCreateProgram = nullptr;
    PFNGLATTACHSHADERPROC            glAttachShader = nullptr;
    PFNGLLINKPROGRAMPROC             glLinkProgram = nullptr;
    PFNGLUSEPROGRAMPROC              glUseProgram = nullptr;
    PFNGLGETSHADERIVPROC             glGetShaderiv = nullptr;
    PFNGLGETPROGRAMIVPROC            glGetProgramiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog = nullptr;
    PFNGLDELETESHADERPROC            glDeleteShader = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv = nullptr;
    PFNGLGENRENDERBUFFERSPROC        glGenRenderbuffers = nullptr;
    PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer = nullptr;
    PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage = nullptr;
    PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D = nullptr;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
    PFNGLTEXIMAGE2DPROC              glTexImage2D = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers = nullptr;
    PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers = nullptr;
    PFNGLDELETEBUFFERSPROC           glDeleteBuffers = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC      glDeleteVertexArrays = nullptr;
    PFNGLDELETEPROGRAMPROC           glDeleteProgram = nullptr;

    void loadOpenGLFunctions() 
    {
        glGenBuffers = (PFNGLGENBUFFERSPROC)glfwGetProcAddress("glGenBuffers");
        glBindBuffer = (PFNGLBINDBUFFERPROC)glfwGetProcAddress("glBindBuffer");
        glBufferData = (PFNGLBUFFERDATAPROC)glfwGetProcAddress("glBufferData");
        glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glfwGetProcAddress("glGenVertexArrays");
        glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glfwGetProcAddress("glBindVertexArray");
        glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glfwGetProcAddress("glVertexAttribPointer");
        glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glfwGetProcAddress("glEnableVertexAttribArray");
        glCreateShader = (PFNGLCREATESHADERPROC)glfwGetProcAddress("glCreateShader");
        glShaderSource = (PFNGLSHADERSOURCEPROC)glfwGetProcAddress("glShaderSource");
        glCompileShader = (PFNGLCOMPILESHADERPROC)glfwGetProcAddress("glCompileShader");
        glCreateProgram = (PFNGLCREATEPROGRAMPROC)glfwGetProcAddress("glCreateProgram");
        glAttachShader = (PFNGLATTACHSHADERPROC)glfwGetProcAddress("glAttachShader");
        glLinkProgram = (PFNGLLINKPROGRAMPROC)glfwGetProcAddress("glLinkProgram");
        glUseProgram = (PFNGLUSEPROGRAMPROC)glfwGetProcAddress("glUseProgram");
        glGetShaderiv = (PFNGLGETSHADERIVPROC)glfwGetProcAddress("glGetShaderiv");
        glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glfwGetProcAddress("glGetProgramiv");
        glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glfwGetProcAddress("glGetShaderInfoLog");
        glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glfwGetProcAddress("glGetProgramInfoLog");
        glDeleteShader = (PFNGLDELETESHADERPROC)glfwGetProcAddress("glDeleteShader");
        glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glfwGetProcAddress("glGetUniformLocation");
        glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)glfwGetProcAddress("glUniformMatrix4fv");
        glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)glfwGetProcAddress("glGenRenderbuffers");
        glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)glfwGetProcAddress("glBindRenderbuffer");
        glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)glfwGetProcAddress("glRenderbufferStorage");
        glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)glfwGetProcAddress("glGenFramebuffers");
        glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)glfwGetProcAddress("glBindFramebuffer");
        glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)glfwGetProcAddress("glFramebufferTexture2D");
        glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)glfwGetProcAddress("glFramebufferRenderbuffer");
        glTexImage2D = (PFNGLTEXIMAGE2DPROC)glfwGetProcAddress("glTexImage2D");
        glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)glfwGetProcAddress("glDeleteFramebuffers");
        glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)glfwGetProcAddress("glDeleteRenderbuffers");
        glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glfwGetProcAddress("glDeleteBuffers");
        glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)glfwGetProcAddress("glDeleteVertexArrays");
        glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glfwGetProcAddress("glDeleteProgram");
    }

    GLuint loadShader(const char* source, GLenum type) 
    {
        
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        return shader;
    }

    GLuint createProgram(const char* vertexSource, const char* fragmentSource) 
    {
        GLuint vertexShader = loadShader(vertexSource, GL_VERTEX_SHADER);
        GLuint fragmentShader = loadShader(fragmentSource, GL_FRAGMENT_SHADER);
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return program;
    }
}

inline const GLfloat* glmValuePtr(const glm::mat4& mat) 
{
    return reinterpret_cast<const GLfloat*>(&mat[0]);
}

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

void setMatrixUniforms(GLuint shaderProgram, const char* name, const GLfloat* matrix) 
{
    GLint location = ogl::glGetUniformLocation(shaderProgram, name);
    ogl::glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
}

void drawScene(GLuint shaderProgram, GLuint vao, LKGCamera& camera, float tx = 0.0f, bool invert = false, float depthiness = 0.09f, float focus = 0.0f)
{
    ogl::glBindVertexArray(vao);
    ogl::glUseProgram(shaderProgram);

    // Compute view and projection matrices using LKGCamera
    float viewMatrix[16];
    float projectionMatrix[16];
    camera.computeViewProjectionMatrices(tx, invert, depthiness, focus, viewMatrix, projectionMatrix);

    // Compute the model matrix (e.g., rotating cube)
    float timeValue = (float)glfwGetTime();
    float modelMatrix[16];
    camera.getModelMatrix(modelMatrix, timeValue, -timeValue);

    // Set uniforms
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, modelMatrix);
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, viewMatrix);
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projectionMatrix);

    // Draw the object
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

int main(void)
{
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 800, "Bridge InProc SDK Native Sample -- No Device Connected!", nullptr, nullptr);
    
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
            if (controller->InstanceWindowGL(&wnd, first_display_id))
            {
                // Window handle created successfully
            }
            else
            {
                wnd = 0;
            }
        }
    }

    // Create BridgeData
    BridgeData bridgeData = BridgeData::Create(*controller, wnd);

    // Update window size and title
    if (bridgeData.wnd != 0)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        if (!bridgeData.display_infos.empty())
        {
            const DisplayInfo& firstDisplay = bridgeData.display_infos.front();
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::string window_title = "Bridge InProc SDK Native Sample -- " +
                                    converter.to_bytes(firstDisplay.name) +
                                    " : " + converter.to_bytes(firstDisplay.serial);
            glfwSetWindowTitle(window, window_title.c_str());
        }

        bridgeData.window_width  = bridgeData.output_width / 2;
        bridgeData.window_height = bridgeData.output_height / 2;

        glfwSetWindowSize(window, bridgeData.window_width, bridgeData.window_height);
    }

    LKGCamera camera = LKGCamera();

    camera.position[0] = 0.0f; camera.position[1] = 0.0f; camera.position[2] = 5.0f;
    camera.target[0] = 0.0f;   camera.target[1] = 0.0f;   camera.target[2] = 0.0f;
    camera.up[0] = 0.0f;       camera.up[1] = 1.0f;       camera.up[2] = 0.0f;
    camera.fov = 45.0f;
    camera.aspectRatio = bridgeData.displayaspect;
    camera.nearPlane = 0.1f;
    camera.farPlane = 100.0f;

    // Initialize OpenGL textures and framebuffers
    GLuint render_texture = 0;
    GLuint render_fbo = 0;
    GLuint depth_buffer = 0;

    if (bridgeData.wnd != 0)
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

    float vertices[] = 
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

    unsigned int indices[] = 
    {
        0, 1, 2, 2, 3, 0,        // Front face
        4, 5, 6, 6, 7, 4,        // Back face
        8, 9, 10, 10, 11, 8,     // Left face
        12, 13, 14, 14, 15, 12,  // Right face
        16, 17, 18, 18, 19, 16,  // Bottom face
        20, 21, 22, 22, 23, 20   // Top face
    };

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
    float depthiness = 0.9f;  // Adjust as needed
    float focus = 0.0f;        // Adjust as needed

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

        if (bridgeData.wnd)
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
                    float tx = (float)viewIndex / (float)(totalViews - 1);

                    drawScene(shaderProgram, vao, camera, tx, true, depthiness, focus);
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
