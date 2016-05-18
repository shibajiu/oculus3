#include "oculusgl.h"

int main(int argc, char **argv){
	
	if (ovr_Initialize(0)){
		init(&argc,argv);
		glfwSetKeyCallback(window, key_callback);
		while (!glfwWindowShouldClose(window)){
			glfwPollEvents();



			glfwSwapBuffers(window);
		}
	}

	
	return 0;
}

int init(int *argc, char **argv){
	//create a glfw window
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	if (!(window = glfwCreateWindow(640, 480, "Oculus", NULL, NULL))){
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);

	//connect to hmd, or create a virtual DK2
	if (!(hmd = ovrHmd_Create(0))){
		fprintf(stderr, "fail to open hmd,falling back to virtual DK2\n");
		if (!(hmd = ovrHmd_CreateDebug(ovrHmd_DK2))){
			fprintf(stderr, "fail to create virtual DK2\n");
			return 0;
		}
	}
	printf("Hello,%d from %s!\n", hmd->ProductId,hmd->SerialNumber);

	// Start the sensor which provides the Rift¡¯s pose and motion.
	if (!(ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Orientation | ovrTrackingCap_Position,0))){
		fprintf(stderr, "configure tracking error\n");
	}

	//target render texture size
	eyeRecommondSize[0] = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, hmd->DefaultEyeFov[0],1);
	eyeRecommondSize[1] = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, hmd->DefaultEyeFov[1], 1);
	renderTargetSize.w = eyeRecommondSize[0].w + eyeRecommondSize[1].w;
	renderTargetSize.h = (eyeRecommondSize[0].h < eyeRecommondSize[1].h) ? eyeRecommondSize[1].h : eyeRecommondSize[0].h;	

	//configure opengl
	cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	cfg.OGL.Header.BackBufferSize.w = hmd->Resolution.w;
	cfg.OGL.Header.BackBufferSize.h = hmd->Resolution.w;
	cfg.OGL.Header.Multisample = 1;
	cfg.OGL.DC = wglGetCurrentDC();
	cfg.OGL.Window = glfwGetWin32Window(window);

	//also generates the ovrEyeRenderDescstructure that describes all of the details needed to perform stereo rendering.
	distortionCaps = ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive;
	if (!(ovrHmd_ConfigureRendering(hmd, &cfg.Config, distortionCaps, hmd->DefaultEyeFov, eyeRDOut))){
		fprintf(stderr, "fail to configure distortion rendering\n");
	}
	ovrHmd_SetEnabledCaps(hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);
	if (hmd->HmdCaps == ovrHmdCap_ExtendDesktop){
		printf("running in ExtendDesktop mode\n");
	}
	else{
		ovrHmd_AttachToWindow(hmd, glfwGetWin32Window(window), 0, 0);
		printf("running in Direct mode\n");
	}

	//ovrTexture initialization
	for (int i = 0; i < 2; ++i){
		eyeTexture[i].OGL.Header.API = ovrRenderAPI_OpenGL;
		eyeTexture[i].OGL.Header.TextureSize = renderTargetSize;
		eyeTexture[i].OGL.Header.RenderViewport.Pos.x = i == 0 ? -eyeRecommondSize[0].w*0.5 : eyeRecommondSize[1].w*0.5;
		eyeTexture[i].OGL.Header.RenderViewport.Pos.y = 0;
		eyeTexture[i].OGL.Header.TextureSize.h = renderTargetSize.h;
		eyeTexture[i].OGL.Header.TextureSize.w = renderTargetSize.w;//?
		eyeTexture[i].OGL.TexId = eyeTextureId;//single texture
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_NORMALIZE);
	glClearColor(0.1, 0.1, 0.1, 1);

	printf("loading texture...\n");
	single_tex_cube = SOIL_load_OGL_cubemap
		(
		"skybox\\right.jpg",
		"skybox\\left.jpg",
		"skybox\\top.jpg",
		"skybox\\bottom.jpg",
		"skybox\\back.jpg",
		"skybox\\front.jpg",
		SOIL_LOAD_RGB,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS
		);
	if (single_tex_cube)
		printf("texture loaded\n");
	else
		printf("fail to load texture\n");

	//system("pause");	
	return 1;
}

void closeHmd(){
	if (window)
		glfwDestroyWindow(window);
	// hmd and LibOVR must be shut down after GLFW
	glfwTerminate();
	if (hmd)
		ovrHmd_Destroy(hmd);	
	ovr_Shutdown();
}

void hswDisplay(){
	// Health and Safety Warning display state.
	ovrHSWDisplayState hswDisplayState;
	ovrHmd_GetHSWDisplayState(hmd, &hswDisplayState);
	if (hswDisplayState.Displayed){
		ovrHmd_DismissHSWDisplay(hmd);
	}
}

void frame_rendering_loop(){
	//frame timing information
	frameTiming = ovrHmd_BeginFrame(hmd, 0);
	//Get eye poses, feeding in correct IPD offset
	ovrVector3f viewOffset[2] = { eyeRDOut[0].HmdToEyeViewOffset, eyeRDOut[1].HmdToEyeViewOffset };
	ovrPosef eyePose[2];
	ovrHmd_GetEyePoses(hmd, 0, viewOffset, eyePose, 0);
	for (int i = 0; i < 2; i++){
		ovrEyeType eye = hmd->EyeRenderOrder[i];
		//eyePose[eye] = ovrHmd_GetHmdPosePerEye(hmd, eye);
		// Get view and projection matrices
		Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(eyePose[eye].Orientation);
		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
		Vector3f shiftedEyePos = Pos2 + rollPitchYaw.Transform(eyePose[eye].Position);

		Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
		Matrix4f proj = ovrMatrix4f_Projection(hmd->DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_RightHanded);
	}
	//submits the eye images for distortion processing
	ovrHmd_EndFrame(hmd, eyePose, &eyeTexture[0].Texture);
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	
	glColor3f(1, 1, 1);//Color Pollution prevention
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBegin(GL_QUADS);
	//-x
	glTexCoord3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(-fExtent, fExtent, -fExtent);
	glTexCoord3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(-fExtent, fExtent, fExtent);
	glTexCoord3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-fExtent, -fExtent, fExtent);
	glTexCoord3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-fExtent, -fExtent, -fExtent);

	//+x
	glTexCoord3f(1.0f, -1.0f, -1.0f);
	glVertex3f(fExtent, -fExtent, -fExtent);
	glTexCoord3f(1.0f, -1.0f, 1.0f);
	glVertex3f(fExtent, -fExtent, fExtent);
	glTexCoord3f(1.0f, 1.0f, 1.0f);
	glVertex3f(fExtent, fExtent, fExtent);
	glTexCoord3f(1.0f, 1.0f, -1.0f);
	glVertex3f(fExtent, fExtent, -fExtent);

	//+y
	glTexCoord3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(-fExtent, fExtent, -fExtent);
	glTexCoord3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(-fExtent, fExtent, fExtent);
	glTexCoord3f(1.0f, 1.0f, 1.0f);
	glVertex3f(fExtent, fExtent, fExtent);
	glTexCoord3f(1.0f, 1.0f, -1.0f);
	glVertex3f(fExtent, fExtent, -fExtent);

	//-y
	glTexCoord3f(1.0f, -1.0f, -1.0f);
	glVertex3f(fExtent, -fExtent, -fExtent);
	glTexCoord3f(1.0f, -1.0f, 1.0f);
	glVertex3f(fExtent, -fExtent, fExtent);
	glTexCoord3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-fExtent, -fExtent, fExtent);
	glTexCoord3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-fExtent, -fExtent, -fExtent);


	//-z
	glTexCoord3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-fExtent, -fExtent, -fExtent);
	glTexCoord3f(1.0f, -1.0f, -1.0f);
	glVertex3f(fExtent, -fExtent, -fExtent);
	glTexCoord3f(1.0f, 1.0f, -1.0f);
	glVertex3f(fExtent, fExtent, -fExtent);
	glTexCoord3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(-fExtent, fExtent, -fExtent);

	//+z
	glTexCoord3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(-fExtent, fExtent, fExtent);
	glTexCoord3f(1.0f, 1.0f, 1.0f);
	glVertex3f(fExtent, fExtent, fExtent);
	glTexCoord3f(1.0f, -1.0f, 1.0f);
	glVertex3f(fExtent, -fExtent, fExtent);
	glTexCoord3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-fExtent, -fExtent, fExtent);

	glEnd();
	glDisable(GL_TEXTURE_CUBE_MAP);
	glColor3f(1.0f, 0.0f, 0.0f);
	
	glFlush();
}

void displayteaport(float scale){
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(0, 0, -10);
	glutWireTeapot(scale);
	glPopMatrix();
}

void reshape(int w, int h){
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 30.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -3.6);
}

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}
static void key_callback(GLFWwindow* window1, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window1, GL_TRUE);
}