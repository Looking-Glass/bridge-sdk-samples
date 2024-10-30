#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <GL/glext.h>
#include <string.h>
#include <bridge.h>
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

struct Dim
{
    unsigned long width  = 0;
    unsigned long height = 0;
};

struct BridgeVersionAndPostfix
{
    unsigned long major = 0;
    unsigned long minor = 0;
    unsigned long build = 0; 
    wstring       postfix;
};

struct Calibration
{
    float center = 0;
    float pitch = 0;
    float slope = 0;
    int width = 0;
    int height = 0;
    float dpi = 0;
    float flip_x = 0;
    int invView = 0;
    float viewcone = 0.0f;
    float fringe = 0.0f;
    int cell_pattern_mode = 0;
    vector<CalibrationSubpixelCell> cells;
};

struct DefaultQuitSettings
{
    float aspect        = 0.0f;
    int   quilt_width   = 0;
    int   quilt_height  = 0;
    int   quilt_columns = 0;
    int   quilt_rows    = 0;
};

struct WindowPos
{
    long x = 0;
    long y = 0;
};

unique_ptr<Controller>      g_controller     = nullptr;
WINDOW_HANDLE               g_wnd            = 0;
unsigned long               g_display_index  = 0;
float                       g_viewcone       = 0.0f;
int                         g_device_type    = 0;
float                       g_aspect         = 0.0f;
int                         g_quilt_width    = 0;
int                         g_quilt_height   = 0;
int                         g_vx             = 0;
int                         g_vy             = 0;
unsigned long               g_output_width   = 800;
unsigned long               g_output_height  = 600;
int                         g_window_width   = 800;
int                         g_window_height  = 600;
int                         g_view_width     = 0;
int                         g_view_height    = 0;
GLuint                      g_render_texture = 0;
GLuint                      g_render_fbo     = 0;
GLuint                      g_depth_buffer   = 0;
int                         g_invview        = 0;
int                         g_ri             = 0;
int                         g_bi             = 0;
float                       g_tilt           = 0.0f;
float                       g_displayaspect  = 0.0f;
float                       g_fringe         = 0.0f;
float                       g_subp           = 0.0f;
float                       g_pitch          = 0.0f;
float                       g_center         = 0.0f;
WindowPos                   g_window_position;
BridgeVersionAndPostfix     g_active_bridge_version;
vector<unsigned long>       g_displays;
vector<wstring>             g_display_serials;
vector<wstring>             g_display_names;
vector<Dim>                 g_display_dimensions;
vector<int>                 g_display_hw_enums;
vector<Calibration>         g_display_calibrations;
vector<int>                 g_display_viewinvs;
vector<int>                 g_display_ris;
vector<int>                 g_display_bis;
vector<float>               g_display_tilts;
vector<float>               g_display_aspects;
vector<float>               g_display_fringes;
vector<float>               g_display_subps;
vector<float>               g_display_viewcones;
vector<float>               g_display_centers;
vector<float>               g_display_pitches;
vector<DefaultQuitSettings> g_display_default_quilt_settings;
vector<WindowPos>           g_display_window_positions;

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

void calculateModelMatrix(GLfloat* matrix, GLfloat angleX, GLfloat angleY) 
{
    GLfloat cosX = cos(angleX);
    GLfloat sinX = sin(angleX);
    GLfloat cosY = cos(angleY);
    GLfloat sinY = sin(angleY);

    // Rotation around X-axis
    GLfloat rotationX[16] = 
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cosX, -sinX, 0.0f,
        0.0f, sinX, cosX, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Rotation around Y-axis
    GLfloat rotationY[16] = 
    {
        cosY, 0.0f, sinY, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -sinY, 0.0f, cosY, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Translation matrix
    GLfloat translation[16] = 
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, -3.0f, 1.0f
    };

    // Temporary matrix for combined rotation
    GLfloat temp[16];

    // Multiply rotationY by rotationX
    for (int i = 0; i < 4; ++i) 
    {
        for (int j = 0; j < 4; ++j) 
        {
            temp[i * 4 + j] = rotationY[i * 4 + 0] * rotationX[0 * 4 + j] +
                              rotationY[i * 4 + 1] * rotationX[1 * 4 + j] +
                              rotationY[i * 4 + 2] * rotationX[2 * 4 + j] +
                              rotationY[i * 4 + 3] * rotationX[3 * 4 + j];
        }
    }

    // Multiply temp by translation
    for (int i = 0; i < 4; ++i) 
    {
        for (int j = 0; j < 4; ++j) 
        {
            matrix[i * 4 + j] = temp[i * 4 + 0] * translation[0 * 4 + j] +
                                temp[i * 4 + 1] * translation[1 * 4 + j] +
                                temp[i * 4 + 2] * translation[2 * 4 + j] +
                                temp[i * 4 + 3] * translation[3 * 4 + j];
        }
    }
}

void calculateViewMatrix(GLfloat* matrix) 
{
    matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f; matrix[3] = 0.0f;
    matrix[4] = 0.0f; matrix[5] = 1.0f; matrix[6] = 0.0f; matrix[7] = 0.0f;
    matrix[8] = 0.0f; matrix[9] = 0.0f; matrix[10] = 1.0f; matrix[11] = -5.0f;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

void calculateProjectionMatrix(GLfloat* matrix, float aspectRatio, float fov, float nearPlane, float farPlane) 
{
    float top = tan(fov / 2) * nearPlane;
    float right = top * aspectRatio;
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = nearPlane / right;
    matrix[5] = nearPlane / top;
    matrix[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    matrix[11] = -1.0f;
    matrix[14] = -(2 * farPlane * nearPlane) / (farPlane - nearPlane);
}

void populate_displays()
{
    int lkg_display_count = 0;
    g_controller->GetDisplays(&lkg_display_count, nullptr);

    g_displays.resize(lkg_display_count);
    g_controller->GetDisplays(&lkg_display_count, g_displays.data());

    for (auto it : g_displays)
    {
        {
            wstring serial;
            int serial_count = 0;

            g_controller->GetDeviceSerialForDisplay(it, &serial_count, nullptr);

            serial.resize(serial_count);
            g_controller->GetDeviceSerialForDisplay(it, &serial_count, serial.data());

            g_display_serials.push_back(serial);
        }

        {
            wstring name;
            int name_count = 0;

            g_controller->GetDeviceNameForDisplay(it, &name_count, nullptr);

            name.resize(name_count);
            g_controller->GetDeviceNameForDisplay(it, &name_count, name.data());

            g_display_names.push_back(name);
        }

        {
            Dim dim;
            g_controller->GetDimensionsForDisplay(it, &dim.width, &dim.height);

            g_display_dimensions.push_back(dim);
        }

        {
            int hw_enum = -1;
            g_controller->GetDeviceTypeForDisplay(it, &hw_enum);

            g_display_hw_enums.push_back(hw_enum);
        }

        {
            Calibration calibration;
            int number_of_cells = 0;

            g_controller->GetCalibrationForDisplay(it,
                                                   &calibration.center,
                                                   &calibration.pitch,
                                                   &calibration.slope,
                                                   &calibration.width,
                                                   &calibration.height,
                                                   &calibration.dpi,
                                                   &calibration.flip_x,
                                                   &calibration.invView,
                                                   &calibration.viewcone,
                                                   &calibration.fringe,
                                                   &calibration.cell_pattern_mode,
                                                   &number_of_cells,
                                                   nullptr);

            calibration.cells.resize(number_of_cells);

            g_controller->GetCalibrationForDisplay(it,
                                                   &calibration.center,
                                                   &calibration.pitch,
                                                   &calibration.slope,
                                                   &calibration.width,
                                                   &calibration.height,
                                                   &calibration.dpi,
                                                   &calibration.flip_x,
                                                   &calibration.invView,
                                                   &calibration.viewcone,
                                                   &calibration.fringe,
                                                   &calibration.cell_pattern_mode,
                                                   &number_of_cells,
                                                   calibration.cells.data());

            g_display_calibrations.push_back(calibration);
        }

        {
            int invview;
            g_controller->GetInvViewForDisplay(it, &invview);

            g_display_viewinvs.push_back(invview);
        }

        {
            int ri = 0;
            g_controller->GetRiForDisplay(it, &ri);

            g_display_ris.push_back(ri);
        }

        {
            int bi = 0;
            g_controller->GetBiForDisplay(it, &bi);

            g_display_bis.push_back(bi);
        }

        {
            float tilt = 0.0f;
            g_controller->GetTiltForDisplay(it, &tilt);

            g_display_tilts.push_back(tilt);
        }

        {
            float aspect = 0.0f;
            g_controller->GetDisplayAspectForDisplay(it, &aspect);

            g_display_aspects.push_back(aspect);
        }

        {
            float fringe = 0.0f;
            g_controller->GetFringeForDisplay(it, &fringe);

            g_display_fringes.push_back(fringe);
        }

        {
            float subp = 0.0f;
            g_controller->GetSubpForDisplay(it, &subp);

            g_display_subps.push_back(subp);
        }

        {
            float viewcone = 0.0f;
            g_controller->GetViewConeForDisplay(it, &viewcone);

            g_display_viewcones.push_back(viewcone);
        }

        {
            float center = 0.0f;
            g_controller->GetCenterForDisplay(it, &center);

            g_display_centers.push_back(center);
        }

        {
            float pitch = 0.0f;
            g_controller->GetPitchForDisplay(it, &pitch);

            g_display_pitches.push_back(pitch);
        }

        {
            DefaultQuitSettings quilt_settings;
            g_controller->GetDefaultQuiltSettingsForDisplay(it, &quilt_settings.aspect, &quilt_settings.quilt_width, &quilt_settings.quilt_height, &quilt_settings.quilt_columns, &quilt_settings.quilt_rows);

            g_display_default_quilt_settings.push_back(quilt_settings);
        }

        {
            WindowPos pos;
            g_controller->GetWindowPositionForDisplay(it, &pos.x, &pos.y);

            g_display_window_positions.push_back(pos);
        }
    }
}

void populate_window_values()
{
    g_controller->GetDisplayForWindow(g_wnd, &g_display_index);
    g_controller->GetDeviceType(g_wnd, &g_device_type);
    g_controller->GetDefaultQuiltSettings(g_wnd, &g_aspect, &g_quilt_width, &g_quilt_height, &g_vx, &g_vy);
    g_controller->GetWindowDimensions(g_wnd, &g_output_width, &g_output_height);
    g_controller->GetViewCone(g_wnd, &g_viewcone);
    g_controller->GetInvView(g_wnd, &g_invview);
    g_controller->GetRi(g_wnd, &g_ri);
    g_controller->GetBi(g_wnd, &g_bi);
    g_controller->GetTilt(g_wnd, &g_tilt);
    g_controller->GetDisplayAspect(g_wnd, &g_displayaspect);
    g_controller->GetFringe(g_wnd, &g_fringe);
    g_controller->GetSubp(g_wnd, &g_subp);
    g_controller->GetPitch(g_wnd, &g_pitch);
    g_controller->GetCenter(g_wnd, &g_center);
    g_controller->GetWindowPosition(g_wnd, &g_window_position.x, &g_window_position.y);
}

void populate_active_bridge_version()
{
    int number_of_postfix_wchars = 0;

    g_controller->GetBridgeVersion(&g_active_bridge_version.major,
                                   &g_active_bridge_version.minor,
                                   &g_active_bridge_version.build,
                                   &number_of_postfix_wchars,
                                   nullptr);

    g_active_bridge_version.postfix.resize(number_of_postfix_wchars);

    g_controller->GetBridgeVersion(&g_active_bridge_version.major,
                                   &g_active_bridge_version.minor,
                                   &g_active_bridge_version.build,
                                   &number_of_postfix_wchars,
                                   g_active_bridge_version.postfix.data());

}

void initBridge(GLFWwindow* window)
{
    g_controller = make_unique<Controller>();

#ifdef _WIN32
    if (g_controller->Initialize(L"BridgeInProcSampleNative"))   
#else
    if (g_controller->Initialize("BridgeInProcSampleNative"))
#endif
    {
        populate_active_bridge_version();
        populate_displays();
 
        if (!g_displays.empty())
        if (g_controller->InstanceWindowGL(&g_wnd, g_displays.front()))
        {
            populate_window_values();

            wstring name;
            int     name_size = 0;
            g_controller->GetDeviceName(g_wnd, &name_size, nullptr);

            name.resize(name_size);
            g_controller->GetDeviceName(g_wnd, &name_size, name.data());

            wstring serial;
            int     serial_size = 0;

            g_controller->GetDeviceSerial(g_wnd, &serial_size, nullptr);

            serial.resize(serial_size);
            g_controller->GetDeviceSerial(g_wnd, &serial_size, serial.data());

            g_window_width  = g_output_width/2;
            g_window_height = g_output_height/2;

            glfwSetWindowSize(window, g_window_width, g_window_height);

            wstring_convert<codecvt_utf8<wchar_t>> converter;
            string window_title = "Bridge InProc SDK Native Sample -- " + converter.to_bytes(name) + " : " + converter.to_bytes(serial);
            glfwSetWindowTitle(window, window_title.c_str());
        }
    }

    if (g_wnd == 0)
    {
        // mlc: couldn't init bridge, run desktop head only
        g_controller = nullptr;
    }
    else
    {
        // mlc: bridge inited, finish setup
        g_view_width  = int(float(g_quilt_width) / float(g_vx));
        g_view_height = int(float(g_quilt_height) / float(g_vy));

        glGenTextures(1, &g_render_texture);
        glBindTexture(GL_TEXTURE_2D, g_render_texture);

        // Set texture parameters
        ogl::glTexImage2D(GL_TEXTURE_2D, 
                     0, 
                     GL_RGBA, 
                     g_quilt_width, 
                     g_quilt_height, 
                     0, 
                     GL_RGBA,
                     GL_UNSIGNED_BYTE, 
                     nullptr); 

        // Generate and bind the renderbuffer for depth
        ogl::glGenRenderbuffers(1, &g_depth_buffer);
        ogl::glBindRenderbuffer(GL_RENDERBUFFER, g_depth_buffer); 

        // Create a depth buffer
        ogl::glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, g_quilt_width, g_quilt_height);

        // Generate the framebuffer
        ogl::glGenFramebuffers(1, &g_render_fbo);
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, g_render_fbo);

        // Attach the texture to the framebuffer as a color attachment
        ogl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_render_texture, 0);

        // Attach the renderbuffer as a depth attachment
        ogl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_depth_buffer);

        ogl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ogl::glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
}

void drawScene(GLuint shaderProgram, GLuint vao, float tx = 0.0f, bool invert = false) 
{
    ogl::glBindVertexArray(vao);
    ogl::glUseProgram(shaderProgram);

    glm::vec3 eye(0, 0, 5); 
    glm::vec3 center(0, 0, 0); 
    glm::vec3 up(0, 1, 0); 

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(-tx, 0, 0)) * glm::lookAt(eye, center, up);

    if (invert) 
    {
        view = glm::scale(view, glm::vec3(1, -1, 1));
    }

    float aspectRatio = float(g_window_width) / g_window_height;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    ogl::glUniformMatrix4fv(ogl::glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

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

    initBridge(window);

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

    while (!glfwWindowShouldClose(window))
    {
        glfwMakeContextCurrent(window);

        // mlc: draw to primary head
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // mlc: retina immune viewport
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);  
        glViewport(0, 0, fbWidth, fbHeight);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float timeValue = (float)glfwGetTime();
        GLfloat modelMatrix[16];
        calculateModelMatrix(modelMatrix, timeValue, -timeValue);

        setMatrixUniforms(shaderProgram, "model", modelMatrix);

        drawScene(shaderProgram, vao);

        glfwSwapBuffers(window);

        if (g_wnd)
        {
            // mlc: we have a window -- draw the quilt views for the hologram!
            ogl::glBindFramebuffer(GL_FRAMEBUFFER, g_render_fbo);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float tx_offset = 0.009f;
            float tx = -(float)(g_vx * g_vy - 1) / 2.0f * tx_offset;

            for (int y = 0; y < g_vy; y++)
            {
                for (int x = 0; x < g_vx; x++)
                {
                    int invertedY = g_vy - 1 - y;
                    glViewport(x * g_view_width, invertedY * g_view_height, g_view_width, g_view_height);

                    drawScene(shaderProgram, vao, tx, true);

                    tx += tx_offset;
                }
            }

            g_controller->DrawInteropQuiltTextureGL(g_wnd, g_render_texture, PixelFormats::RGBA, g_quilt_width, g_quilt_height, g_vx, g_vy, g_displayaspect, 1.0f);
        }
        
        glfwPollEvents();
    }

    if (g_controller)
    {
        g_controller->Uninitialize();
    }

    ogl::glDeleteVertexArrays(1, &vao);
    ogl::glDeleteBuffers(1, &vbo);
    ogl::glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &g_render_texture);
    ogl::glDeleteRenderbuffers(1, &g_depth_buffer);
    ogl::glDeleteFramebuffers(1, &g_render_fbo);
    ogl::glDeleteProgram(shaderProgram);

    glfwTerminate();

    return 0;
}
