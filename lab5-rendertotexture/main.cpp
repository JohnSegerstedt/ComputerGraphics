#include <GL/glew.h>


// STB_IMAGE for loading images of many filetypes
#include <stb_image.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <Model.h>
#include "hdr.h"

using std::min;
using std::max;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////
SDL_Window* g_window = nullptr;
float currentTime = 0.0f;

///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////
GLuint backgroundProgram, shaderProgram, postFxShader;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
float environment_multiplier = 1.0f;
GLuint environmentMap, irradianceMap, reflectionMap;
const std::string envmap_base_name = "001";

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
float point_light_intensity_multiplier = 1000.0f;
vec3 point_light_color = vec3(1.f, 1.f, 1.f);
const vec3 lightPosition = vec3(20.0f, 40.0f, 0.0f);

///////////////////////////////////////////////////////////////////////////////
// Camera parameters.
///////////////////////////////////////////////////////////////////////////////
vec3 securityCamPos = vec3(70.0f, 50.0f, -70.0f);
vec3 securityCamDirection = normalize(-securityCamPos);
vec3 cameraPosition(-70.0f, 50.0f, 70.0f);
vec3 cameraDirection = normalize(vec3(0.0f) - cameraPosition);

vec3 worldUp(0.0f, 1.0f, 0.0f);

///////////////////////////////////////////////////////////////////////////////
// World camera.
///////////////////////////////////////////////////////////////////////////////
vec3 cameraOffset(50.0f, 20.0f, 0.0f);
vec3 cameraDirectionOffsetDefault(-1.0f, -0.3f, 0.0f);
//vec3 cameraDirectionOffset(-1.0f, -0.3f, 0.0f);
vec3 cameraDirectionOffset = cameraDirectionOffsetDefault;

vec3 cameraRight;
vec3 cameraUp;

struct PerspectiveParams
{
	float fov;
	int w;
	int h;
	float near;
	float far;
};


static PerspectiveParams pp = { 60.0f, 1280, 720, 0.1f, 10000.0f };

mat4 shipModelMatrix = mat4(1.0f);

float shipSpeedDefault = 3.0f;
float shipSpeed = shipSpeedDefault;
float shipBoostSpeedDefault = 5.0f;
float shipTurnSpeed = 0.05f;
float shipStrafeSpeed = 3.0f;
float shipUpSpeed = 0.5f;
float cameraZoomSpeed = 1.1f;

static mat4 T = mat4(1.0f);
static mat4 R = mat4(1.0f);


///////////////////////////////////////////////////////////////////////////////
// Particle System
///////////////////////////////////////////////////////////////////////////////
#include "ParticleSystem.h"

GLuint vertexArrayObject;
GLuint particleProgram;
ParticleSystem particleSystem = ParticleSystem(1000);


///////////////////////////////////////////////////////////////////////////////
// Models
///////////////////////////////////////////////////////////////////////////////
const std::string model_filename = "../scenes/NewShip.obj";
labhelper::Model *landingpadModel = nullptr;
labhelper::Model *fighterModel = nullptr;
labhelper::Model *sphereModel = nullptr;
labhelper::Model *cameraModel = nullptr;


///////////////////////////////////////////////////////////////////////////////
// Framebuffers
///////////////////////////////////////////////////////////////////////////////

struct FboInfo;
std::vector<FboInfo> fboList;

struct FboInfo {
	GLuint framebufferId;
	GLuint colorTextureTarget;
	GLuint depthBuffer;
	int width;
	int height;
	bool isComplete;

	FboInfo(int w, int h) {
		isComplete = false;
		width = w;
		height = h;
		// Generate two textures and set filter parameters (no storage allocated yet)
		glGenTextures(1, &colorTextureTarget);
		glBindTexture(GL_TEXTURE_2D, colorTextureTarget);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenTextures(1, &depthBuffer);
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// allocate storage for textures
		resize(width, height);

		///////////////////////////////////////////////////////////////////////
		// Generate and bind framebuffer
		///////////////////////////////////////////////////////////////////////
		// >>> @task 1
		glGenFramebuffers(1, &framebufferId);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

		// bind the texture as color attachment 0 (to the currently bound framebuffer)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureTarget, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		// bind the texture as depth attachment (to the currently bound framebuffer)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

		// check if framebuffer is complete
		isComplete = checkFramebufferComplete();

		// bind default framebuffer, just in case.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// if no resolution provided
	FboInfo() : isComplete(false)
		, framebufferId(UINT32_MAX)
		, colorTextureTarget(UINT32_MAX)
		, depthBuffer(UINT32_MAX)
		, width(0)
		, height(0)
	{};

	void resize(int w, int h) {
		width = w;
		height = h;
		// Allocate a texture
		glBindTexture(GL_TEXTURE_2D, colorTextureTarget);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		// generate a depth texture
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	}

	bool checkFramebufferComplete(void) {
		// Check that our FBO is correctly set up, this can fail if we have
		// incompatible formats in a buffer, or for example if we specify an
		// invalid drawbuffer, among things.
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			labhelper::fatal_error("Framebuffer not complete");
		}
		
		return (status == GL_FRAMEBUFFER_COMPLETE);
	}
};


void initGL()
{
	// enable Z-buffering
	glEnable(GL_DEPTH_TEST);

	// enable backface culling
	glEnable(GL_CULL_FACE);

	// Load some models.
	landingpadModel = labhelper::loadModelFromOBJ("../scenes/landingpad.obj");
	cameraModel = labhelper::loadModelFromOBJ("../scenes/camera.obj");
	fighterModel = labhelper::loadModelFromOBJ("../scenes/NewShip.obj");

	// load and set up default shader
	backgroundProgram = labhelper::loadShaderProgram("../acg/shaders/background.vert", "../acg/shaders/background.frag");
	shaderProgram     = labhelper::loadShaderProgram("../acg/shaders/simple.vert",     "../acg/shaders/simple.frag");
	postFxShader      = labhelper::loadShaderProgram("../acg/shaders/postFx.vert",     "../acg/shaders/postFx.frag");
	particleProgram      = labhelper::loadShaderProgram("../acg/shaders/basicVertexShader.vert",     "../acg/shaders/basicFragmentShader.frag");
	//particleProgram      = labhelper::loadShaderProgram("../acg/shaders/particle.vert",     "../acg/shaders/particle.frag");

	///////////////////////////////////////////////////////////////////////////
	// Load environment map
	///////////////////////////////////////////////////////////////////////////
	const int roughnesses = 8;
	std::vector<std::string> filenames;
	for (int i = 0; i < roughnesses; i++)
		filenames.push_back("../scenes/envmaps/" + envmap_base_name + "_dl_" + std::to_string(i) + ".hdr");

	reflectionMap = labhelper::loadHdrMipmapTexture(filenames);
	environmentMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + ".hdr");
	irradianceMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + "_irradiance.hdr");

	///////////////////////////////////////////////////////////////////////////
	// Setup Framebuffers
	///////////////////////////////////////////////////////////////////////////
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);
	const int numFbos = 5;
	for (int i = 0; i < numFbos; i++)
		fboList.push_back(FboInfo(w, h));
	

	// newCode
	//////////////////////////////////////////////////////////////////////////////
	// INSERT DATA INTO PARTICLESYSTEM
	//////////////////////////////////////////////////////////////////////////////
	const unsigned int numberOfParticlesToSpawn = 100;
	const float scale = 10.0f;
	
	for (int i = 0; i < numberOfParticlesToSpawn; i++) {
		const float theta = labhelper::uniform_randf(0.f, 2.f * M_PI);
		const float u = labhelper::uniform_randf(-1.f, 1.f);
		glm::vec3 pos = glm::vec3(sqrt(1.f - u * u) * cosf(theta) * scale, u * scale, sqrt(1.f - u * u) * sinf(theta) * scale);
		Particle spawnedParticle = Particle(1.0f, 0.0f, vec3(0.0f), pos);
		particleSystem.spawn(spawnedParticle);
	}

	//////////////////////////////////////////////////////////////////////////////
	// EXTRACT DATA FROM PARTICLESYSTEM
	//////////////////////////////////////////////////////////////////////////////
	unsigned int active_particles = particleSystem.particles.size();
	
	std::vector<glm::vec4> data;
	data.resize(active_particles);


	for (unsigned int i = 0; i < active_particles; i++) {
		Particle extractedParticle = particleSystem.particles[i];
		data[i] = vec4(extractedParticle.position.x, extractedParticle.position.y, extractedParticle.position.z, extractedParticle.life_length);
	}

	// sort particles with sort from c++ standard library
	std::sort(data.begin(), std::next(data.begin(), active_particles),
		[](const vec4 &lhs, const vec4 &rhs) { return lhs.z < rhs.z; });

	//////////////////////////////////////////////////////////////////////////////
	// BINDING BUFFERS AND SHADERS
	//////////////////////////////////////////////////////////////////////////////
	glUseProgram(particleProgram);
	glBufferData(GL_ARRAY_BUFFER, 100000, nullptr, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, particleSystem.particles.size() * sizeof(vec4), &data);

	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);
	glDrawArrays(GL_POINTS, 0, active_particles);
	

}

void drawScene(const mat4 &view, const mat4 &projection)
{
	glUseProgram(backgroundProgram);
	labhelper::setUniformSlow(backgroundProgram, "environment_multiplier", environment_multiplier);
	labhelper::setUniformSlow(backgroundProgram, "inv_PV", inverse(projection * view));
	labhelper::setUniformSlow(backgroundProgram, "camera_pos", cameraPosition);
	labhelper::drawFullScreenQuad();

	glUseProgram(shaderProgram);
	// Light source
	vec4 viewSpaceLightPosition = view * vec4(lightPosition, 1.0f);
	labhelper::setUniformSlow(shaderProgram, "point_light_color", point_light_color);
	labhelper::setUniformSlow(shaderProgram, "point_light_intensity_multiplier", point_light_intensity_multiplier);
	labhelper::setUniformSlow(shaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));

	// Environment
	labhelper::setUniformSlow(shaderProgram, "environment_multiplier", environment_multiplier);

	// camera
	labhelper::setUniformSlow(shaderProgram, "viewInverse", inverse(view));

	// landing pad 
	mat4 modelMatrix(1.0f);
	modelMatrix[3] += vec4(0.0f, -10.0f, 0.0f, 0.0f);
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projection * view * modelMatrix);
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", view * modelMatrix);
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * modelMatrix)));
	
	labhelper::render(landingpadModel);

	// Fighter
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projection * view * shipModelMatrix);
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", view * shipModelMatrix);
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * shipModelMatrix)));

	labhelper::render(fighterModel);

	// newCode
	glUseProgram(shaderProgram);
	labhelper::setUniformSlow(shaderProgram, "viewProjectionMatrix", projection * view);
}



void display()
{
	///////////////////////////////////////////////////////////////////////////
	// Check if any framebuffer needs to be resized
	///////////////////////////////////////////////////////////////////////////
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);

	for (int i = 0; i < fboList.size(); i++) {
		if (fboList[i].width != w || fboList[i].height != h)
			fboList[i].resize(w, h);
	}

	///////////////////////////////////////////////////////////////////////////
	// setup matrices
	///////////////////////////////////////////////////////////////////////////
	mat4 securityCamViewMatrix = lookAt(securityCamPos, securityCamPos + securityCamDirection, worldUp);
	mat4 securityCamProjectionMatrix = perspective(radians(30.0f), float(w) / float(h), 15.0f, 1000.0f);

	//mat4 projectionMatrix = perspective(radians(45.0f), float(w) / float(h), 10.0f, 1000.0f);

	cameraRight = normalize(cross(cameraDirection, worldUp));
	cameraUp = normalize(cross(cameraRight, cameraDirection));
	mat3 cameraBaseVectorsWorldSpace(cameraRight, cameraUp, -cameraDirection);

	mat4 cameraRotation = mat4(transpose(cameraBaseVectorsWorldSpace));
	mat4 viewMatrix = cameraRotation * translate(-cameraPosition);;
	mat4 projectionMatrix = perspective(radians(pp.fov), float(pp.w) / float(pp.h), pp.near, pp.far);
	//mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraDirection, worldUp);

	///////////////////////////////////////////////////////////////////////////
	// Bind the environment map(s) to unused texture units
	///////////////////////////////////////////////////////////////////////////
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, environmentMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, reflectionMap);

	///////////////////////////////////////////////////////////////////////////
	// draw scene from security camera
	///////////////////////////////////////////////////////////////////////////
	// >>> @task 2
	FboInfo &securityFB0 = fboList[0]; // normal camera
	FboInfo &securityFB1 = fboList[1]; // security camera

	glBindFramebuffer(GL_FRAMEBUFFER, securityFB1.framebufferId);

	glViewport(0, 0, securityFB1.width, securityFB1.height);
	glClearColor(0.2, 0.2, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene(securityCamViewMatrix, securityCamProjectionMatrix); // using both shaderProgram and backgroundProgram

	///////////////////////////////////////////////////////////////////////////
	// draw scene from camera
	///////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, securityFB0.framebufferId); // to be replaced with another framebuffer when doing post processing
	glViewport(0, 0, w, h);
	glClearColor(0.2, 0.2, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene(viewMatrix, projectionMatrix); // using both shaderProgram and backgroundProgram


	///////// TV SCREEN - SECURITY CAMERA
	labhelper::Material &screen = landingpadModel->m_materials[8];
	screen.m_emission_texture.gl_id = securityFB1.colorTextureTarget;

	// camera (obj-model)
	glUseProgram(shaderProgram);
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * inverse(securityCamViewMatrix) * scale(vec3(10.0f)) * rotate(float(M_PI), vec3(0.0f, 1.0, 0.0)));
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", viewMatrix * inverse(securityCamViewMatrix));
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(viewMatrix * inverse(securityCamViewMatrix))));
	
	labhelper::render(cameraModel);


	
	///////////////////////////////////////////////////////////////////////////
	// Post processing pass(es)
	///////////////////////////////////////////////////////////////////////////

	// task 3 - WHAT WE ARE SEEING (NOW CAMERA 0 - NORMAL)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, securityFB0.colorTextureTarget);

	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(postFxShader);

	/*
	labhelper::setUniformSlow(postFxShader, "time", currentTime);
	labhelper::setUniformSlow(postFxShader, "currentEffect", currentEffect);
	labhelper::setUniformSlow(postFxShader, "filterSize", filterSizes[filterSize - 1]);
	labhelper::setUniformSlow(postFxShader, "mosaicSize", mosaicSizes[mosaicSize - 1]);
	*/
	labhelper::drawFullScreenQuad();
	


	glUseProgram(0);

	CHECK_GL_ERROR();
}

bool handleEvents(void)
{
	// check events (keyboard among other)
	SDL_Event event;
	bool quitEvent = false;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE)) {
			quitEvent = true;
		}

		// Allow ImGui to capture events.
		ImGui_ImplSdlGL3_ProcessEvent(&event);

		if (event.type == SDL_MOUSEMOTION && !ImGui::IsMouseHoveringAnyWindow()) {
			// More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
			static int prev_xcoord = event.motion.x;
			static int prev_ycoord = event.motion.y;
			int delta_x = event.motion.x - prev_xcoord;
			int delta_y = event.motion.y - prev_ycoord;
			if (event.button.button & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				float rotationSpeed = 0.005f;
				mat4 yaw = rotate(rotationSpeed * -delta_x, worldUp);
				mat4 pitch = rotate(rotationSpeed * -delta_y, normalize(cross(cameraDirectionOffset, worldUp)));
				cameraDirectionOffset = vec3(pitch * yaw * vec4(cameraDirectionOffset, 0.0f));
				printf("Mouse motion while left button down (%i, %i)\n", event.motion.x, event.motion.y);
			}
			prev_xcoord = event.motion.x;
			prev_ycoord = event.motion.y;
		}

		/*
		if (event.type == SDL_MOUSEMOTION && !ImGui::IsMouseHoveringAnyWindow()) {
			// More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
			static int prev_xcoord = event.motion.x;
			static int prev_ycoord = event.motion.y;
			int delta_x = event.motion.x - prev_xcoord;
			int delta_y = event.motion.y - prev_ycoord;

			if (event.button.button & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				float rotationSpeed = 0.005f;
				mat4 yaw = rotate(rotationSpeed * -delta_x, worldUp);
				mat4 pitch = rotate(rotationSpeed * -delta_y, normalize(cross(cameraDirection, worldUp)));
				cameraDirection = vec3(pitch * yaw * vec4(cameraDirection, 0.0f));
			}

			if (event.button.button & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
				float rotationSpeed = 0.005f;
				mat4 yaw = rotate(rotationSpeed * -delta_x, worldUp);
				mat4 pitch = rotate(rotationSpeed * -delta_y, normalize(cross(securityCamDirection, worldUp)));
				securityCamDirection = vec3(pitch * yaw * vec4(securityCamDirection, 0.0f));
			}

			prev_xcoord = event.motion.x;
			prev_ycoord = event.motion.y;
			*/

	}

	// check keyboard state (which keys are still pressed)
	const uint8_t *state = SDL_GetKeyboardState(nullptr);
	vec3 cameraRight = cross(cameraDirection, worldUp);
	
	// implement controls based on key states
	//static mat4 R = initializeR();
	if (state[SDL_SCANCODE_UP]) {
		cameraOffset /= cameraZoomSpeed;
	}
	if (state[SDL_SCANCODE_DOWN]) {
		cameraOffset *= cameraZoomSpeed;
		//cameraPosition -= normalize(cameraDirection) * cameraMoveSpeed;
	}
	if (state[SDL_SCANCODE_LSHIFT]) {
		shipSpeed = shipBoostSpeedDefault;
	}
	else {
		shipSpeed = shipSpeedDefault;
	}
	if (state[SDL_SCANCODE_W]) {
		T[3] -= shipSpeed * R[0];
	}
	if (state[SDL_SCANCODE_S]) {
		T[3] += shipSpeed * R[0];
	}
	if (state[SDL_SCANCODE_A]) {
		T[3] += shipStrafeSpeed * R[2];
	}
	if (state[SDL_SCANCODE_D]) {
		T[3] -= shipStrafeSpeed * R[2];
	}
	if (state[SDL_SCANCODE_Q]) {
		R[0] -= shipTurnSpeed * R[2];
	}
	if (state[SDL_SCANCODE_E]) {
		R[0] += shipTurnSpeed * R[2];
	}
	if (state[SDL_SCANCODE_SPACE]) {
		cameraDirectionOffset = cameraDirectionOffsetDefault;
	}
	/* if (state[SDL_SCANCODE_SPACE]) {
		T[3] += shipUpSpeed * R[1];
	}*/

	R[0] = normalize(R[0]);
	R[2] = vec4(cross(vec3(R[0]), vec3(R[1])), 0.0f);
	shipModelMatrix = T * R;
	cameraPosition = vec3(T[3]) + mat3(R) * cameraOffset;
	cameraDirection = normalize(mat3(R) * cameraDirectionOffset);

	return quitEvent;
}


void gui()
{
	/*
	// Inform imgui of new frame
	ImGui_ImplSdlGL3_NewFrame(g_window);

	// ----------------- Set variables --------------------------
	ImGui::Text("Post-processing effect");
	ImGui::RadioButton("None", &currentEffect, PostProcessingEffect::None);
	ImGui::RadioButton("Sepia", &currentEffect, PostProcessingEffect::Sepia);
	ImGui::RadioButton("Mushroom", &currentEffect, PostProcessingEffect::Mushroom);
	ImGui::RadioButton("Blur", &currentEffect, PostProcessingEffect::Blur);
	ImGui::SameLine();
	ImGui::SliderInt("Filter size", &filterSize, 1, 12);
	ImGui::RadioButton("Grayscale", &currentEffect, PostProcessingEffect::Grayscale);
	ImGui::RadioButton("All of the above", &currentEffect, PostProcessingEffect::Composition);
	ImGui::RadioButton("Mosaic", &currentEffect, PostProcessingEffect::Mosaic);
	ImGui::SameLine();
	ImGui::SliderInt("Mosaic size", &mosaicSize, 1, 11);
	ImGui::RadioButton("Separable Blur", &currentEffect, PostProcessingEffect::Separable_blur);
	ImGui::RadioButton("Bloom", &currentEffect, PostProcessingEffect::Bloom);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	// ----------------------------------------------------------

	// Render the GUI.
	ImGui::Render();
	*/
}

int main(int argc, char *argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Lab 5");

	initGL();

	bool stopRendering = false;
	auto startTime = std::chrono::system_clock::now();

	while (!stopRendering) {
		//update currentTime
		std::chrono::duration<float> timeSinceStart = std::chrono::system_clock::now() - startTime;
		currentTime = timeSinceStart.count();

		// render to window
		display();

		// Render overlay GUI.
		gui();

		// Swap front and back buffer. This frame will now been displayed.
		SDL_GL_SwapWindow(g_window);

		// check events (keyboard among other)
		stopRendering = handleEvents();
	}

	// Free Models
	labhelper::freeModel(landingpadModel);
	labhelper::freeModel(cameraModel);
	labhelper::freeModel(fighterModel);
	labhelper::freeModel(sphereModel);

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}
