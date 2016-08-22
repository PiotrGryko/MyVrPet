#include <GLES2/gl2.h>

#include <android/log.h>
#include <malloc.h>
#include "../opencl/CLcommon.hpp"
#include <vector>

using namespace std;
//
// Created by piotr on 05/05/16.
//


GLuint vertexShader;
GLuint fragmentShader;
GLuint programObject;
GLint linked;

static float ORTHO_MATRIX[16] = {0.0012f, 0, 0, 0, 0, -7.8, 0, -1,
                                 0, 0, -1, 0, 0, 0, 0, 1};

static float mModelMatrix[16];
static float mViewMatrix[16];
static float mProjectionMatrix[16];

static int mPositionHandle = 0;
static int mColorHandle = 1;
GLint mModelHandle;
GLint mViewHandle;
GLint mProjectionHandle;
GLint mMVPMatrixHandle;


static const char *vShaderStr = "attribute vec4 vPosition;   \n"
        "attribute vec4 vColor;      \n"
        "varying vec4 color;      \n"
        "uniform mat4 uMMatrix;    \n"
        "uniform mat4 uVMatrix;    \n"
        "uniform mat4 uPMatrix;    \n"
        "uniform mat4 uMVPMatrix;    \n"

        "void main()                 \n"
        "{                           \n"
        "   color = vColor;			\n"
        "   gl_Position = uPMatrix *vPosition; \n"
        "}                           \n";

static const char *fShaderStr =
        "precision mediump float;                  \n"
                "varying vec4 color;      \n"
                "void main()                               \n"
                "{                                         \n"
                " gl_FragColor = color; \n"
                "}                                         \n";




GLuint LoadShader(GLenum type, const char *shaderSrc) {
   LOGD( "SDL", "laod shader ..",
                        glGetError());

    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader(type);
    if (shader == 0) {
        LOGD( "SDL", "shader ==0 return...",
                            glGetError());

        return 0;
    }
    LOGD("SDL", "shader created",
                        glGetError());

    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);

    // Compile the shader
    glCompileShader(shader);

    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            std::vector<GLchar> errorLog(infoLen);

            glGetShaderInfoLog(shader, infoLen, &infoLen, &errorLog[0]);
            // esLogMessage("Error compiling shader:\n%s\n", infoLog);
            //   free(infoLog);
        }
        glDeleteShader(shader);
        LOGD( "SDL",
                            "shader complilation failed retuning..", glGetError());

        return 0;
    }
    LOGD( "SDL", "shader compiled",
                        glGetError());

    return shader;
}
static void setOrtho(float left, float right, float bottom, float top,
                     float near, float far) {

    float r_width = 1.0f / (right - left);
    float r_height = 1.0f / (top - bottom);
    float r_depth = 1.0f / (far - near);
    float x = 2.0f * (r_width);
    float y = 2.0f * (r_height);
    float z = -2.0f * (r_depth);
    float tx = -(right + left) * r_width;
    float ty = -(top + bottom) * r_height;
    float tz = -(far + near) * r_depth;
    ORTHO_MATRIX[0] = x;
    ORTHO_MATRIX[5] = y;
    ORTHO_MATRIX[10] = z;
    ORTHO_MATRIX[12] = tx;
    ORTHO_MATRIX[13] = ty;
    ORTHO_MATRIX[14] = tz;
    ORTHO_MATRIX[15] = 1.0f;
    ORTHO_MATRIX[1] = 0.0f;
    ORTHO_MATRIX[2] = 0.0f;
    ORTHO_MATRIX[3] = 0.0f;
    ORTHO_MATRIX[4] = 0.0f;
    ORTHO_MATRIX[6] = 0.0f;
    ORTHO_MATRIX[7] = 0.0f;
    ORTHO_MATRIX[8] = 0.0f;
    ORTHO_MATRIX[9] = 0.0f;
    ORTHO_MATRIX[11] = 0.0f;

    int i = 0;
    for (i = 0; i < 16; i++) {
        LOGD("ORTHO MATRIX index=%d value=%f", i, ORTHO_MATRIX[i]);
    }
}

void Engine_setup_opengl(int width, int height) {

    //glEnable(GL_DEPTH_TEST);

    LOGD("init setup open",
                        glGetError());
    // Create the program object
    programObject = glCreateProgram();
    LOGD("program created",
                        glGetError());

    if (programObject == 0) {
        LOGE( "program ==0 return",
                            glGetError());

        return;
    }
    vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);
    LOGD("shaders laoded",
                        glGetError());

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    // Bind vPosition to attribute 0
    glBindAttribLocation(programObject, mPositionHandle, "vPosition");
    // Bind vColor to attribute 1
    glBindAttribLocation(programObject, mColorHandle, "vColor");
    // glBindUniformLocation(programObject, mMVPMatrixHandle, "uMVPMatrix");
    // Link the program
    glLinkProgram(programObject);
    LOGD( "program linking...",
                        glGetError());

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            std::vector<GLchar> errorLog(infoLen);
            glGetProgramInfoLog(programObject, infoLen, NULL, &errorLog[0]);
            // esLogMessage("Error linking program:\n%s\n", infoLog);
            LOGD( "eror linking program",
                                glGetError());
        //    free(infoLog);
        }

        glDeleteProgram(programObject);

        LOGD(
                            "program not linked... deleting...", glGetError());

        return;
    }
    // Store the program object
    // userData->programObject = programObject;

    // Set the viewport
    LOGD( "program linked",
                        glGetError());

    //glViewport(0, 0, width, height);

    // glOrtho(0, width,height, 0, -1, 1);

    // ORTHO_MATRIX[0]=width;
    // ORTHO_MATRIX[5]=height;

     setOrtho(0, (float)width, (float)height, 0, -1, 1);
}



void Engine_on_surface_changed(int width, int height) {
    glViewport(0, 0, width, height);
    // setOrtho(0, (float)width, (float)height, 0, -1, 1);
}

void Engine_prepare_draw() {
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.2f, 0.9f, 1.0f);
}

/////////////////////////////////builder//////////////////////////////////////////////////////////

vector<float> vectorVertex;
vector<float> vectorColors;
GLenum vDrawingType;

float alpha = 1;
float red = 0;
float green = 0;
float blue = 0;

void Engine_glBegin(GLenum type) {

    vDrawingType = type;

}

void Engine_glVertex2d(float x, float y) {
    vectorVertex.push_back(x);
    vectorVertex.push_back(y);
    vectorVertex.push_back(0);

    vectorColors.push_back(red);
    vectorColors.push_back(green);
    vectorColors.push_back(blue);
    vectorColors.push_back(alpha);

}

void Engine_glLineWidth(float size) { }

void Engine_glVertex3f(float x, float y, float z) {

    vectorVertex.push_back(x);
    vectorVertex.push_back(y);
    vectorVertex.push_back(z);

    vectorColors.push_back(red);
    vectorColors.push_back(green);
    vectorColors.push_back(blue);
    vectorColors.push_back(alpha);

}

void Engine_glVertex3fArray(float* array)
{
    //vectorVertex.insert(std::end(vectorVertex), std::begin(&array), std::end(&array));

}

void Engine_glColor3f(float r, float g, float b) {
    red = r;
    green = g;
    blue = b;
}

void Engine_setMVPMatrices(float *projection, float *view, float *model)
{
    for(int i=0;i<16;i++)
    {
        mModelMatrix[i]=model[i];
        mViewMatrix[i]=view[i];
        mProjectionMatrix[i]=projection[i];

    }


}

void Engine_glEnd() {


    GLfloat vVertices[vectorVertex.size()];
    GLfloat vColors[vectorColors.size()];
    int i;
    for (i = 0; i < vectorVertex.size(); i++) {
        vVertices[i] = vectorVertex[i];
    }
    for (i = 0; i < vectorColors.size(); i++) {
        vColors[i] = vectorColors[i];
    }

    // Use the program object
    glUseProgram(programObject);
    mModelHandle = glGetUniformLocation(programObject, "uMMatrix");
    glUniformMatrix4fv(mModelHandle, 1, 0, mModelMatrix);

    mViewHandle = glGetUniformLocation(programObject, "uVMatrix");
    glUniformMatrix4fv(mViewHandle, 1, 0, mViewMatrix);

    mProjectionHandle = glGetUniformLocation(programObject, "uPMatrix");
    glUniformMatrix4fv(mProjectionHandle, 1, 0, mProjectionMatrix);

    mMVPMatrixHandle = glGetUniformLocation(programObject, "uMVPMatrix");
    glUniformMatrix4fv(mMVPMatrixHandle, 1, 0, ORTHO_MATRIX);


    // Load the vertex data

    glVertexAttribPointer(mPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(mPositionHandle);
    // Load color data

    glVertexAttribPointer(mColorHandle, 4, GL_FLOAT, GL_FALSE, 0, vColors);
    glEnableVertexAttribArray(mColorHandle);

    glDrawArrays(vDrawingType, 0, vectorVertex.size() / 3);
    vectorColors.clear();
    vectorVertex.clear();

}