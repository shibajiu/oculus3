#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <SOIL.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras\OVR_Math.h>



#ifdef WIN32
#define OVR_OS_WIN32
#endif // WIN32

using namespace OVR;

ovrHmd hmd;
ovrSizei eyeRecommondSize[2];
ovrSizei renderTargetSize;
ovrGLConfig cfg;
static GLuint single_tex_cube;
static unsigned int distortionCaps, eyeTextureId;
ovrEyeRenderDesc eyeRDOut[2];
ovrFrameTiming frameTiming;
ovrGLTexture eyeTexture[2];
static float Yaw(3.141592f);
static Vector3f Pos2(0.0f, 1.6f, -5.0f);
static GLfloat fExtent = 10.0f;

int init(int *argc, char **argv);
void closeHmd();
void hswDisplay();
void frame_rendering_loop();
void reshape(int w, int h);
void display(void);