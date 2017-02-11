/*Copyright 2016 Google
Author: Opaque Media Group

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/

#include "TangoPluginPrivatePCH.h"
#include "TangoDeviceImage.h"
#include "TangoDevice.h"
#include "Async/ParallelFor.h"
#if PLATFORM_ANDROID
#include "GLES/gl.h"
#include <tango_client_api.h>

// adapted from tango_unity_lib

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES/glext.h>
//#include <IUnityInterface.h>
//#include <IUnityGraphics.h>
#include <android/log.h>
//#include <glog/logging.h>

namespace {

	/// Vertices used for rendering AR texture.
	const float kArVertices[] = { -1.0, -1.0, 0.0, -1.0, +1.0, 0.0,
		+1.0, -1.0, 0.0, +1.0, +1.0, 0.0 };

	/// Indexes used for rendering AR texture.
	const int kArIndexes[] = { 1, 0, 2, 1, 2, 3 };

	/// Vertex shader used for rendering AR texture in GL2.
	const char* kArVertexShaderGL2 =
		"#version 100\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"attribute vec3 vertex;\n"
		"attribute vec2 uv;\n"
		"varying vec2 f_uv;\n"
		"void main() {\n"
		"  f_uv = uv;\n"
		"  gl_Position = vec4(vertex, 1);\n"
		"}\n";

	/// Vertex shader used for rendering AR texture in GL3.
	const char* kArVertexShaderGL3 =
		"#version 300 es\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"in vec3 vertex;\n"
		"in vec2 uv;\n"
		"out vec2 f_uv;\n"
		"void main() {\n"
		"  f_uv = uv;\n"
		"  gl_Position = vec4(vertex, 1);\n"
		"}\n";

	/// Fragment shader used for rendering AR texture in GL2.
	const char* kArFragmentShaderGL2 =
		"#version 100\n"
		"#extension GL_OES_EGL_image_external : require\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"uniform samplerExternalOES ar_texture;\n"
		"varying vec2 f_uv;\n"
		"void main() {\n"
		"  gl_FragColor = texture2D(ar_texture, f_uv);\n"
		"}\n";

	/// Fragment shader used for rendering AR texture in GL3.
	const char* kArFragmentShaderGL3 =
		"#version 300 es\n"
		"#extension GL_OES_EGL_image_external_essl3 : require\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"uniform samplerExternalOES ar_texture;\n"
		"in vec2 f_uv;\n"
		"out vec4 gl_FragColor;\n"
		"void main() {\n"
		"  gl_FragColor = texture(ar_texture, f_uv);\n"
		"}\n";

	/// Fragment shader used for rendering AR texture with
	/// rectification in GL2.
	const char* kArFragmentShaderDistortionGL2 =
		"#version 100\n"
		"#extension GL_OES_EGL_image_external : require\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"uniform samplerExternalOES ar_texture;\n"
		"uniform vec2 ar_texture_size;\n"
		"uniform vec2 f;\n"
		"uniform vec2 c;\n"
		"uniform vec3 k;\n"
		"varying vec2 f_uv;\n"
		"void main() {\n"
		"  vec2 xy = (f_uv * ar_texture_size - c) / f;\n"
		"  float r2 = dot(xy, xy);\n"
		"  float icdist = 1.0 + r2 * (k.x + r2 * (k.y + r2 * k.z));\n"
		"  vec2 undistorted_xy = xy * icdist;\n"
		"  vec2 uv = (undistorted_xy * f + c) / ar_texture_size;\n"
		"  gl_FragColor = texture2D(ar_texture, uv);\n"
		"}\n";

	/// Fragment shader used for rendering AR texture with
	/// rectification in GL3.
	const char* kArFragmentShaderDistortionGL3 =
		"#version 300 es\n"
		"#extension GL_OES_EGL_image_external_essl3 : require\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"uniform samplerExternalOES ar_texture;\n"
		"uniform vec2 ar_texture_size;\n"
		"uniform vec2 f;\n"
		"uniform vec2 c;\n"
		"uniform vec3 k;\n"
		"in vec2 f_uv;\n"
		"out vec4 gl_FragColor;\n"
		"void main() {\n"
		"  vec2 xy = (f_uv * ar_texture_size - c) / f;\n"
		"  float r2 = dot(xy, xy);\n"
		"  float icdist = 1.0 + r2 * (k.x + r2 * (k.y + r2 * k.z));\n"
		"  vec2 undistorted_xy = xy * icdist;\n"
		"  vec2 uv = (undistorted_xy * f + c) / ar_texture_size;\n"
		"  gl_FragColor = texture(ar_texture, uv);\n"
		"}\n";

	/// Becomes true once initialization has happened.
	bool g_initialized = false;

	/// If rendering uses the distortion path or not.
	bool g_use_distortion;

	/// The color camera intrinsics to use for distortion.
	TangoCameraIntrinsics g_distortion_intrinsics;

	/// The texture used to do AR rendering.
	GLuint g_ar_texture;

	/// The programs used to render AR textures.
	GLuint g_ar_program;
	GLuint g_ar_program_distortion;

	/// Uniforms and attributes for `g_ar_program`.
	GLint g_ar_program_ar_texture_uniform;
	GLint g_ar_program_vertex_attrib;
	GLint g_ar_program_uv_attrib;

	/// Uniforms and attributes for `g_ar_program_distortion`.
	GLint g_ar_program_distortion_ar_texture_uniform;
	GLint g_ar_program_distortion_ar_texture_size_uniform;
	GLint g_ar_program_distortion_f_uniform;
	GLint g_ar_program_distortion_c_uniform;
	GLint g_ar_program_distortion_k_uniform;
	GLint g_ar_program_distortion_vertex_attrib;
	GLint g_ar_program_distortion_uv_attrib;

	// The framebuffer used for environment map rendering;
	GLuint g_environment_map_frame_buffer;
	bool g_environment_map_initialized = false;

	/// The UV for AR rendering.  Defaults to full screen.
	float g_ar_uv[8] = { 0, 0, 0, 1, 1, 0, 1, 1 };

}  // namespace

namespace tango_unity {

	bool CompileShader(int type, const char* vshader_code, GLuint* Result)
	{
		GLuint vshader = glCreateShader(type);
		GLint vshader_compiled;
		glShaderSource(vshader, 1, &vshader_code, NULL);
		glCompileShader(vshader);
		glGetShaderiv(vshader, GL_COMPILE_STATUS, &vshader_compiled);
		if (!vshader_compiled) {
			TArray<char> errorLog;
			errorLog.SetNumUninitialized(8192);
			int len;
			glGetShaderInfoLog(vshader, 8192, &len, errorLog.GetData());
			UE_LOG(TangoPlugin, Error, TEXT("Shader did not compile: %s"), UTF8_TO_TCHAR(errorLog.GetData()));
			return false;
		}
		*Result = vshader;
		return true;
	}


	void AttemptGlInitialize(int gl_version) {
		if (g_initialized) {
			//LOG(INFO) << __func__ << " already completed once.  Skipping.";
			return;
		}

		const char* vshader_code;
		const char* fshader_code;
		const char* fshader_distortion_code;
		if (gl_version == 2) {
			//LOG(INFO) << __func__ << " using GL2 shaders.";
			UE_LOG(TangoPlugin, Log, TEXT("Using GL2 shaders"));
			vshader_code = kArVertexShaderGL2;
			fshader_code = kArFragmentShaderGL2;
			fshader_distortion_code = kArFragmentShaderDistortionGL2;
		}
		else if (gl_version == 3) {
			//LOG(INFO) << __func__ << " using GL3 shaders.";
			UE_LOG(TangoPlugin, Log, TEXT("Using GL3 shaders"));
			vshader_code = kArVertexShaderGL3;
			fshader_code = kArFragmentShaderGL3;
			fshader_distortion_code = kArFragmentShaderDistortionGL3;
		}
		else {
			//LOG(ERROR) << __func__ << " Unrecognized gl version: " << gl_version
			//	<< ", aborting.";
			UE_LOG(TangoPlugin, Error, TEXT("Unrecognized gl version %d"), gl_version);
			return;
		}

		GLuint vshader;
		if (!CompileShader(GL_VERTEX_SHADER, vshader_code, &vshader)) {
			UE_LOG(TangoPlugin, Error, TEXT("Vertex shader did not compile"));
			glDeleteShader(vshader);
			return;
		}

		GLuint fshader;
		if (!CompileShader(GL_FRAGMENT_SHADER, fshader_code, &fshader)) {
			UE_LOG(TangoPlugin, Error, TEXT("Fragment shader did not compile"));
			glDeleteShader(vshader);
			glDeleteShader(fshader);
			return;
		}

		GLuint fshader_distortion;
		if (!CompileShader(GL_FRAGMENT_SHADER, fshader_distortion_code, &fshader_distortion)) {
			UE_LOG(TangoPlugin, Error, TEXT("Fragment shader (distortion) did not compile"));
			glDeleteShader(vshader);
			glDeleteShader(fshader);
			glDeleteShader(fshader_distortion);
			return;
		}

		GLuint program = glCreateProgram();
		GLint program_linked;
		glAttachShader(program, vshader);
		glAttachShader(program, fshader);
		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
		if (!program_linked) {
			//LOG(INFO) << __func__ << " Program did not link, status=" << program_linked;
			UE_LOG(TangoPlugin, Error, TEXT("Program did not link, status=%d"), program_linked);
			glDeleteShader(vshader);
			glDeleteShader(fshader);
			glDeleteShader(fshader_distortion);
			glDeleteProgram(program);
			return;
		}

		GLuint program_distortion = glCreateProgram();
		GLint program_distortion_linked;
		glAttachShader(program_distortion, vshader);
		glAttachShader(program_distortion, fshader_distortion);
		glLinkProgram(program_distortion);
		glGetProgramiv(program_distortion, GL_LINK_STATUS,
			&program_distortion_linked);
		if (!program_distortion_linked) {
			//LOG(INFO) << __func__ << " Program (distortion) did not link, status="
			//	<< program_distortion_linked;
			UE_LOG(TangoPlugin, Error, TEXT("Program (distortion) did not link, status=%d"), program_distortion_linked);
			glDeleteShader(vshader);
			glDeleteShader(fshader);
			glDeleteShader(fshader_distortion);
			glDeleteProgram(program);
			glDeleteProgram(program_distortion);
			return;
		}

		// At this point, that could fail on a specific version is done.

		glGenTextures(1, &g_ar_texture);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_ar_texture);
		UE_LOG(TangoPlugin, Log, TEXT("g_ar_texture=%d"), g_ar_texture);
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		g_ar_program = program;
		g_ar_program_ar_texture_uniform = glGetUniformLocation(program, "ar_texture");
		g_ar_program_vertex_attrib = glGetAttribLocation(program, "vertex");
		g_ar_program_uv_attrib = glGetAttribLocation(program, "uv");

		g_ar_program_distortion = program_distortion;
		g_ar_program_distortion_ar_texture_uniform =
			glGetUniformLocation(program_distortion, "ar_texture");
		g_ar_program_distortion_ar_texture_size_uniform =
			glGetUniformLocation(program_distortion, "ar_texture_size");
		g_ar_program_distortion_f_uniform =
			glGetUniformLocation(program_distortion, "f");
		g_ar_program_distortion_c_uniform =
			glGetUniformLocation(program_distortion, "c");
		g_ar_program_distortion_k_uniform =
			glGetUniformLocation(program_distortion, "k");
		g_ar_program_distortion_vertex_attrib =
			glGetAttribLocation(program_distortion, "vertex");
		g_ar_program_distortion_uv_attrib =
			glGetAttribLocation(program_distortion, "uv");

		g_initialized = true;

		while (GLenum err = glGetError()) {
			//LOG(ERROR) << __func__ << " While initializing, GL error code=" << err;
			UE_LOG(TangoPlugin, Error, TEXT("While initializing, GL error code=%d"), err);
		}
	}

}  // namespace tango_unity

   /// Called when this plugin is first loaded.
extern "C" void
UnityPluginLoad() {
	// Unity's interfaces do not properly give you the correct GL
	// version in Unity 5.2.  Attempt each version until one works.
	if (!g_initialized) {
		tango_unity::AttemptGlInitialize(3);
	}
	if (!g_initialized) {
		tango_unity::AttemptGlInitialize(2);
	}
	if (!g_initialized) {
		//LOG(ERROR) << "Unable to initialize a GLES version.";
		UE_LOG(TangoPlugin, Error, TEXT("Unable to initialize a GLES version."));
	}
}

extern "C" int TangoUnity_getArTexture() {
	// TODO(jfinder): When adding multithreading support add locking
	// around ar texture access.
	return g_ar_texture;
}

/// Set the render texture UVs for future calls to
/// RenderTexture().
///
/// Parameters are in the following order:
/// * Bottom-left
/// * Top-left
/// * Bottom-right
/// * Top-right
extern "C" void TangoUnity_setRenderTextureUVs(float uv[8]) {
	// TODO(jfinder): When adding multithreaded support, add locking
	// around uv access.
	memcpy(g_ar_uv, uv, sizeof(g_ar_uv));
}

/// Set the render texture distortion intrinsics for future calls to
/// RenderTexture().
extern "C" void TangoUnity_setRenderTextureDistortion(
	TangoCameraIntrinsics* intrinsics) {
	// TODO(jfinder): When adding multithreaded support, add locking
	// around distortion access.
	if (intrinsics) {
		g_distortion_intrinsics = *intrinsics;
	}
	g_use_distortion = (intrinsics != nullptr);
}

/// Render the AR texture.
///
/// This is not intended to be exposed via Unity, it should instead
/// get returnned from getRenderArTexture, below.
extern "C" void TangoUnity_renderArTexture(int /*i*/) {
	// Unity sometimes has error states before it calls our function.
	while (GLenum err = glGetError()) {
		//LOG(INFO) << __func__
		//	<< " GL error before entering the function, code=" << err;
	}

	// Some GL state is undefined by Unity.  Get to a consistent state.
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);

	// Render a single fullscreen quad.
	// TODO(jfinder): When adding multithreaded support add locking
	// around texture and uv access.
	if (!g_use_distortion) {
		glUseProgram(g_ar_program);

		glUniform1i(g_ar_program_ar_texture_uniform, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_ar_texture);

		glEnableVertexAttribArray(g_ar_program_vertex_attrib);
		glVertexAttribPointer(g_ar_program_vertex_attrib, 3, GL_FLOAT, GL_FALSE, 0,
			kArVertices);

		glEnableVertexAttribArray(g_ar_program_uv_attrib);
		glVertexAttribPointer(g_ar_program_uv_attrib, 2, GL_FLOAT, GL_FALSE, 0,
			g_ar_uv);
	}
	else {
		glUseProgram(g_ar_program_distortion);

		glUniform1i(g_ar_program_distortion_ar_texture_uniform, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_ar_texture);

		glUniform2f(g_ar_program_distortion_ar_texture_size_uniform,
			g_distortion_intrinsics.width, g_distortion_intrinsics.height);
		glUniform2f(g_ar_program_distortion_f_uniform, g_distortion_intrinsics.fx,
			g_distortion_intrinsics.fy);
		glUniform2f(g_ar_program_distortion_c_uniform, g_distortion_intrinsics.cx,
			g_distortion_intrinsics.cy);
		glUniform3f(g_ar_program_distortion_k_uniform,
			g_distortion_intrinsics.distortion[0],
			g_distortion_intrinsics.distortion[1],
			g_distortion_intrinsics.distortion[2]);

		glEnableVertexAttribArray(g_ar_program_distortion_vertex_attrib);
		glVertexAttribPointer(g_ar_program_distortion_vertex_attrib, 3, GL_FLOAT,
			GL_FALSE, 0, kArVertices);
		glEnableVertexAttribArray(g_ar_program_distortion_uv_attrib);
		glVertexAttribPointer(g_ar_program_distortion_uv_attrib, 2, GL_FLOAT,
			GL_FALSE, 0, g_ar_uv);
	}

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, kArIndexes);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
	while (GLenum err = glGetError()) {
		//LOG(ERROR) << __func__ << " While initializing, GL error code=" << err;
		UE_LOG(TangoPlugin, Error, TEXT("While rendering, GL error code=%d"), err);
	}
}

FCriticalSection g_ArTextureLock;

void(*g_glDrawBuffers)(int, GLenum*) = nullptr;

/// Updates the environment map.
extern "C" void TangoUnity_updateEnvironmentMap(int tex, int width,
	int height) {
	if (!g_environment_map_initialized) {
		glGenFramebuffers(1, &g_environment_map_frame_buffer);
		g_environment_map_initialized = true;
	}
	int glInt;
	bool glBool;
	// Save and reset state.
	bool previousBlend = glIsEnabled(GL_BLEND);
	bool previousCullFace = glIsEnabled(GL_CULL_FACE);
	bool previousScissorTest = glIsEnabled(GL_SCISSOR_TEST);
	bool previousStencilTest = glIsEnabled(GL_STENCIL_TEST);
	bool previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
	bool previousDither = glIsEnabled(GL_DITHER);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &glInt);
	int previousFBO = glInt;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &glInt);
	int previousVBO = glInt;
	int previousViewport[4];
	glGetIntegerv(GL_VIEWPORT, previousViewport);
	glActiveTexture(GL_TEXTURE0);
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &glInt);
	int previousMinFilter = glInt;
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &glInt);
	int previousMagFilter = glInt;
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D,
		GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,
		GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//UE_LOG(TangoPlugin, Log, TEXT("Initialized Frame Buffer"));
	// Bind the framebuffer to the environment map.
	glBindFramebuffer(GL_FRAMEBUFFER, g_environment_map_frame_buffer);
	//UE_LOG(TangoPlugin, Log, TEXT("Bound Frame Buffer"));
	// Set "tex" as our colour attachement #0
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		tex, 0);
	//UE_LOG(TangoPlugin, Log, TEXT("FrameBufferTexture2D %d"), tex);
	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	if (g_glDrawBuffers == nullptr) // hack, glDrawBuffers is null and causes SEGV
	{
		g_glDrawBuffers = (void(*)(int, GLenum*)) eglGetProcAddress("glDrawBuffers");
	}
	g_glDrawBuffers(1, DrawBuffers);  // "1" is the size of DrawBuffers
	//UE_LOG(TangoPlugin, Log, TEXT("Draw Buffer"));
	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		//LOG(ERROR) << __func__ << " Framebuffer error.";
		UE_LOG(TangoPlugin, Error, TEXT("Framebuffer error"));
		return;
	}

	// Render on the whole framebuffer, complete from the lower left corner to
	// the upper right
	glViewport(0, 0, width, height);

	// Render the AR screen to the framebuffer.
	TangoUnity_renderArTexture(0);

	// Restore state and cleanup.
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
	glBindBuffer(GL_ARRAY_BUFFER, previousVBO);

	glViewport(previousViewport[0], previousViewport[1],
		previousViewport[2], previousViewport[3]);
	if (previousBlend) glEnable(GL_BLEND);
	if (previousCullFace) glEnable(GL_CULL_FACE);
	if (previousScissorTest) glEnable(GL_SCISSOR_TEST);
	if (previousStencilTest) glEnable(GL_STENCIL_TEST);
	if (previousDepthTest) glEnable(GL_DEPTH_TEST);
	if (previousDither) glEnable(GL_DITHER);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, previousMinFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, previousMagFilter);
}

#endif



void UTangoDeviceImage::Init(
#if PLATFORM_ANDROID
	TangoConfig Config_
#endif
)
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage: Constructor called"));

	State = DISCONNECTED;
	
	bNewDataAvailable = false;
	bNeedsAllocation = true;
	LastTimestamp = 0;
	RGBOpenGLPointer = 0;
#if PLATFORM_ANDROID
	TangoBuffer.width = 0;
	TangoBuffer.height = 0;
	CreateTexture(Config_);
#endif

	UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage: Constructor finished"));
}

bool UTangoDeviceImage::CreateTexture(
#if PLATFORM_ANDROID
	TangoConfig Config_
#endif
)
{
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage: CreateTexture called"));
#if PLATFORM_ANDROID

	TangoBuffer.width = 0;
	TangoBuffer.height = 0;
	VideoTexture = UTexture2D::CreateTransient(1, 1, PF_R8G8B8A8);
	VideoTexture->UpdateResource();
#endif
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage: CreateTexture FINISHED"));
	return true;
}

bool UTangoDeviceImage::setRuntimeConfig(FTangoRuntimeConfig & RuntimeConfig)
{
	if (RuntimeConfig.bEnableColorCamera)
	{
		if (State == DISCONNECTED)
		{
			ConnectCallback();
		}
		return true;
	}
	else if (State == CONNECTED)
	{
		return DisconnectCallback();
	}
	else
	{
		State = DISCONNECTED;
		return true;
	}
}

bool UTangoDeviceImage::DisconnectCallback()
{
	if (State == DISCONNECTED)
	{
		return true;
	}
	else
	{
		bool bSuccess = true;
#if PLATFORM_ANDROID
		bSuccess = TangoService_disconnectCamera(TANGO_CAMERA_COLOR) == TANGO_SUCCESS;
#endif
		if (bSuccess == true)
		{
			State = DISCONNECTED;
		}
		return bSuccess;
	}
}

void UTangoDeviceImage::OnNewDataAvailable()
{
	bNewDataAvailable = true;
}

void UTangoDeviceImage::ConnectCallback()
{
	State = WANTTOCONNECT;
}

bool UTangoDeviceImage::TexturesReady()
{

	if (!VideoTexture || !VideoTexture->IsValidLowLevel() || !VideoTexture->Resource || !VideoTexture->TextureReference.IsInitialized_GameThread())
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoDeviceImage::TexturesReady: UTangoDeviceImage: VideoTexture not ready"));
		return false;
	}
	return true;

}

void UTangoDeviceImage::CheckConnectCallback()
{
	if (State != WANTTOCONNECT)
	{
		return;
	}
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage::CheckConnectCallback: UTangoDeviceImage: RegisterCallbacks called"))
#if PLATFORM_ANDROID

	if (!TexturesReady())
	{
		return;
	}
	State = CONNECTSHEDULED;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(ConnectTangoCallback,
		FTextureRHIRef, Tex, VideoTexture->Resource->TextureRHI,
		ConnectionState*, StateRef, &State,
		{
			if (*StateRef != CONNECTSHEDULED)
			{
				return;
			}
			if (!Tex)
			{
				UE_LOG(TangoPlugin, Error, TEXT("UTangoDeviceImage::CheckConnectCallback: UTangoDeviceImage: RegisterCallbacks Not TextureRHI"));
				*StateRef = WANTTOCONNECT;
				return;
			}
			void* Res = Tex->GetNativeResource();
			if (Res == nullptr)
			{
				UE_LOG(TangoPlugin, Error, TEXT("UTangoDeviceImage::CheckConnectCallback: UTangoDeviceImage: RegisterCallbacks Not GetNativeResource"));
				*StateRef = WANTTOCONNECT;
				return;
			}
			UTangoDevice::Get().GetTangoDeviceImagePointer()->RGBOpenGLPointer = static_cast<uint32>(*reinterpret_cast<int32*>(Res));
			UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage::CheckConnectCallback: Registering Callback"));
			if (*StateRef == CONNECTSHEDULED)
			{
				*StateRef = CONNECTED;

				UnityPluginLoad();
				TangoErrorType Status = TangoService_connectTextureId(TANGO_CAMERA_COLOR, TangoUnity_getArTexture(), this,
					[](void*, TangoCameraId id) {if (id == TANGO_CAMERA_COLOR && UTangoDevice::Get().GetTangoDeviceImagePointer() != nullptr) UTangoDevice::Get().GetTangoDeviceImagePointer()->OnNewDataAvailable(); }
				);
				if (Status != TANGO_SUCCESS)
				{
					UE_LOG(TangoPlugin, Error, TEXT("UTangoDeviceImage::connectTextureId failed"));
				}
				else
				{
					UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage:: registered texture id  callback"));
				}
				{
					Status = TangoService_connectOnFrameAvailable(TANGO_CAMERA_COLOR, nullptr,
						[](void*, TangoCameraId id, const TangoImageBuffer* buffer) {
						if (id == TANGO_CAMERA_COLOR && UTangoDevice::Get().GetTangoDeviceImagePointer() != nullptr)
						{
							UTangoDevice::Get().GetTangoDeviceImagePointer()->OnImageBuffer(buffer);
						}
					});
					if (Status != TANGO_SUCCESS)
					{
						UE_LOG(TangoPlugin, Error, TEXT("UTangoDeviceImage::connectFrameAvailable failed"));
					}
					else
					{
						UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage:: registered frame available callback"));
					}
				}
			}
		});

#endif
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage::CheckConnectCallback: UTangoDeviceImage: RegisterCallbacks FINISHED"));
}

#if PLATFORM_ANDROID
void UTangoDeviceImage::OnImageBuffer(const TangoImageBuffer* InBuffer)
{
	if (State != CONNECTED)
	{
		return;
	}
	if (OnImageBufferAvailable.IsBound())
	{
		OnImageBufferAvailable.Broadcast(InBuffer);
	}
}

#endif

bool UTangoDeviceImage::IsNewDataAvail()
{
	if (State != CONNECTED)
	{
		return false;
	}
	return bNewDataAvailable;
}

void UTangoDeviceImage::TickByDevice()
{
#if PLATFORM_ANDROID
	CheckConnectCallback();
#endif
}

void UTangoDeviceImage::TickByCamera(TFunction<void(double)> fun)
{
#if PLATFORM_ANDROID
	if (IsNewDataAvail())
	{
		if (TangoBuffer.width == 0 && RGBOpenGLPointer != 0 && bNeedsAllocation)
		{
			if (TangoARHelpers::DataIsReady())
			{
				auto Intrin = TangoARHelpers::GetARCameraIntrinsics();
				TangoBuffer.width = Intrin.Width;
				TangoBuffer.height = Intrin.Height;
				glBindTexture(GL_TEXTURE_2D, RGBOpenGLPointer);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Intrin.Width, Intrin.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				bNeedsAllocation = false;
				TangoCameraIntrinsics TangoIntrinsics;
				TangoIntrinsics.width = Intrin.Width;
				TangoIntrinsics.height = Intrin.Height;
				TangoIntrinsics.cx = Intrin.Cx;
				TangoIntrinsics.cy = Intrin.Cy;
				TangoIntrinsics.fx = Intrin.Fx;
				TangoIntrinsics.fy = Intrin.Fy;
				for (int32 i = 0; i < 5; i++)
				{
					TangoIntrinsics.distortion[i] = Intrin.Distortion[i];
				}
				TangoIntrinsics.calibration_type = (TangoCalibrationType)Intrin.CalibrationType	;
				TangoUnity_setRenderTextureDistortion(&TangoIntrinsics);
				UE_LOG(TangoPlugin, Log, TEXT("Allocated RGB Texture"));
			}
		}
		double Stamp = 0.0;
		FScopeLock ScopeLock(&g_ArTextureLock);
		if (TangoService_updateTextureExternalOes(TANGO_CAMERA_COLOR, TangoUnity_getArTexture(), &Stamp) != TANGO_SUCCESS)
		{
			UE_LOG(TangoPlugin, Error, TEXT("TangoService_updateTexture failed"));
		}
		else
		{
			if (!bNeedsAllocation)
			{
				if (Stamp == LastTimestamp)
				{
					fun(Stamp);
				}
				else
				{
					DataSet(Stamp);
					fun(Stamp);
					ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(UpdateRGBTex,
						int32, Width, TangoBuffer.width,
						int32, Height, TangoBuffer.height,
						int32, Tex, RGBOpenGLPointer,
						double, Timestamp, Stamp,
						{
							FScopeLock LambdaScopeLock(&g_ArTextureLock);
							TangoUnity_updateEnvironmentMap(Tex, Width, Height);
							//UE_LOG(TangoPlugin, Log, TEXT("On Render thread: %f"), Timestamp);
						});
					//UE_LOG(TangoPlugin, Log, TEXT("On Game Thread: %f"), Stamp);
				}
			}
		}
	}
#endif
}

void UTangoDeviceImage::BeginDestroy()
{
	Super::BeginDestroy();
	UE_LOG(TangoPlugin, Log, TEXT("UTangoDeviceImage::BeginDestroy: destructor called"));
	DisconnectCallback();
}

float UTangoDeviceImage::GetImageBufferTimestamp()
{
	float ReturnValue = 0;
#if PLATFORM_ANDROID
	ReturnValue = LastTimestamp;
#endif
	return ReturnValue;
}