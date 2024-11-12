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
typedef void (*PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (*PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
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
    PFNGLACTIVETEXTUREPROC           glActiveTexture = nullptr;
    PFNGLUNIFORM1IPROC               glUniform1i = nullptr;

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
        glActiveTexture = (PFNGLACTIVETEXTUREPROC)glfwGetProcAddress("glActiveTexture");
        glUniform1i = (PFNGLUNIFORM1IPROC)glfwGetProcAddress("glUniform1i");
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

inline void setMatrixUniforms(GLuint shaderProgram, const char* name, const GLfloat* matrix) 
{
    GLint location = ogl::glGetUniformLocation(shaderProgram, name);
    ogl::glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
}
