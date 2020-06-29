
#ifndef YOUME_GLESUtil_h
#define YOUME_GLESUtil_h

#if MAC_OS
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/gl3.h>
#else
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/ES3/gl.h>
#endif

#if MAC_OS
#define DEBUG
#endif



#define BUFFER_OFFSET(i) ((void*)(i))
#define BUFFER_OFFSET_POSITION BUFFER_OFFSET(0)
#define BUFFER_OFFSET_TEXTURE  BUFFER_OFFSET(8)
#define BUFFER_SIZE_POSITION 2
#define BUFFER_SIZE_TEXTURE  2
#define BUFFER_STRIDE (sizeof(float) * 4)



#define GL_ERRORS(line)
#define GL_FRAMEBUFFER_STATUS(line)



static const float s_vbo [] =
{
    -1.f, -1.f,       0.f, 0.f, // 0
    1.f, -1.f,        1.f, 0.f, // 1
    -1.f, 1.f,        0.f, 1.f, // 2
    
    1.f, -1.f,        1.f, 0.f, // 1
    1.f, 1.f,         1.f, 1.f, // 3
    -1.f, 1.f,        0.f, 1.f, // 2
};

static const float squareVertices[] = {
    -1.0f,  -1.0f,
    1.0f,  -1.0f,
    -1.0f,   1.0f,
    1.0f,   1.0f,
};


static const float coordVertices[] = {
    0.0f,  1.0f,
    1.0f,  1.0f,
    0.0f,  0.0f,
    1.0f,  0.0f,
};


static const char s_vs [] =
"attribute vec2 aPos;"
"attribute vec2 aCoord;"
"varying vec2 vCoord;"
"void main(void) {"
"gl_Position = vec4(aPos,0.,1.);"
"vCoord = aCoord;"
"}";
static const char s_vs_mat [] =
"attribute vec2 aPos;"
"attribute vec2 aCoord;"
"varying vec2 vCoord;"
"uniform mat4 uMat;"
"void main(void) {"
"gl_Position = uMat * vec4(aPos,0.,1.);"
"vCoord = aCoord;"
"}";

static const char s_fs[] =
#if !TARGET_OS_OSX
"precision mediump float;"
#endif
"varying vec2 vCoord;"
"uniform sampler2D uTex0;"
"void main(void) {"
#if !TARGET_OS_OSX
"gl_FragData[0] = texture2D(uTex0, vCoord);"
#else
"gl_FragData[0] = texture2D(uTex0, vCoord).bgra;"
#endif
"}";


static const char s_vs_mat_yuv [] =
"attribute vec4 aPos;"
"attribute vec2 aCoord;"
"varying vec2 vCoord;"
"uniform mat4 uMat;"
"void main(void) {"
"gl_Position = uMat * aPos;"
"vCoord = aCoord;"
"}";

static const char s_fs_yuv[] =
#if !TARGET_OS_OSX
"precision mediump float;"
"varying lowp vec2 vCoord;"
#else
"varying  vec2 vCoord;"
#endif

"uniform sampler2D SamplerY;"
"uniform sampler2D SamplerU;"
"uniform sampler2D SamplerV;"
"void main(void)"
"{"
#if !TARGET_OS_OSX
"mediump vec3 yuv;"
"lowp vec3 rgb;"
#else
" vec3 yuv;"
" vec3 rgb;"
#endif
"yuv.x = texture2D(SamplerY, vCoord).r;"
"yuv.y = texture2D(SamplerU, vCoord).r - 0.5;"
"yuv.z = texture2D(SamplerV, vCoord).r - 0.5;"
"rgb = mat3( 1,       1,         1,"
"0,       -0.39465,  2.03211,"
"1.13983, -0.58060,  0) * yuv;"
"gl_FragColor = vec4(rgb, 1);"
"}";






static inline GLuint compile_shader(GLuint type, const char * source)
{
    
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
#ifdef DEBUG
    if (!compiled) {
        GLint length;
        char *log;
        
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        
        log = (char*)malloc((size_t)(length));
        glGetShaderInfoLog(shader, length, &length, &log[0]);
        printf("%s compilation error: %s\n", (type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER"), log);
        free(log);
        
        return 0;
    }
#endif
    return shader;
    
}

static inline GLuint build_program(const char * vertex, const char * fragment)
{
    GLuint  vshad,
    fshad,
    p;
    
    GLint   len;
#ifdef DEBUG
    char*   log;
#endif
    
    vshad = compile_shader(GL_VERTEX_SHADER, vertex);
    fshad = compile_shader(GL_FRAGMENT_SHADER, fragment);
    
    p = glCreateProgram();
    glAttachShader(p, vshad);
    glAttachShader(p, fshad);
    glLinkProgram(p);
    glGetProgramiv(p,GL_INFO_LOG_LENGTH, &len);
    
    
#ifdef DEBUG
    if(len) {
        log = (char*)malloc ( (size_t)(len) ) ;
        
        glGetProgramInfoLog(p, len, &len, log);
        
        printf("program log: %s\n", log);
        free(log);
    }
#endif
    
    
    glDeleteShader(vshad);
    glDeleteShader(fshad);
    return p;
}


#endif
