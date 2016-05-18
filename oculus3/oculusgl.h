#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define OVR_OS_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#define OVR_OS_MAC
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#define OVR_OS_LINUX
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <GL/glut.h>
#include <SOIL.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras\OVR_Math.h>

using namespace OVR;

ovrHmd hmd;
static ovrSizei eyeRecommondSize[2];
static ovrSizei renderTargetSize;
static ovrGLConfig cfg;
static GLuint single_tex_cube;
static unsigned int distortionCaps, eyeTextureId;
static ovrEyeRenderDesc eyeRDOut[2];
static ovrFrameTiming frameTiming;
static ovrGLTexture eyeTexture[2];
static float Yaw(3.141592f);
static Vector3f Pos2(0.0f, 1.6f, -5.0f);
static GLfloat fExtent = 10.0f;
static GLFWwindow *window;

int init(int *argc, char **argv);
void closeHmd();
void hswDisplay();
void frame_rendering_loop();
void reshape(int w, int h);
void display(void);
static void error_callback(int error, const char* description);
static void key_callback(GLFWwindow* window1, int key, int scancode, int action, int mods);