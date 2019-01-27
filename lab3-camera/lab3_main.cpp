#include <GL/glew.h>

#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <chrono>

#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>
#include <Model.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;
using namespace labhelper;

using std::min;
using std::max;

static int old_w = 1280;
static int old_h = 720;

struct PerspectiveParams
{
    float fov;
    int w;
    int h;
    float near;
    float far;
};

static PerspectiveParams pp = {45.0f, 1280, 720, 0.1f, 300.0f};

// The window we'll be rendering to
SDL_Window* g_window = nullptr;

GLuint shaderProgram;

float currentTime = 0.0f;
float testValue = 0.0f;

// Models
Model *cityModel = nullptr, *carModel = nullptr, *carModel2 = nullptr;


mat4 carModelMatrix = mat4(1.0f);
mat4 carModelMatrix2 = mat4(1.0f);

vec3 cameraRight;
vec3 cameraUp;

vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);

// Camera parameters
vec3 cameraPosition(10.0f, 10.0f, 10.0f);
vec3 cameraDirection(-1.0f, -1.0f, -1.0f);
float cameraMoveSpeed = 1.05f;

vec3 cameraOffset(0.0f, 7.0f, -12.0f);
vec3 cameraDirectionOffset(0.0f, -0.3f, 1.0f);

void loadModels()
{
	///////////////////////////////////////////////////////////////////////////
	// Load models (both vertex buffers and textures). 
	///////////////////////////////////////////////////////////////////////////
	cityModel = loadModelFromOBJ("../scenes/city.obj");
	carModel = loadModelFromOBJ("../scenes/car.obj");
	carModel2 = loadModelFromOBJ("../scenes/car.obj");
	
}

void display(void)
{
	// Set up
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);

        if (pp.w != old_w || pp.h != old_h)
        {
            SDL_SetWindowSize(g_window, pp.w, pp.h);
            w = pp.w;
            h = pp.h;
            old_w = pp.w;
            old_h = pp.h;
        }

	glViewport(0, 0, w, h);		// Set viewport
	glClearColor(0.2, 0.2, 0.8, 1.0);					// Set clear color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the color buffer and the z-buffer
	glEnable(GL_DEPTH_TEST);							// enable Z-buffering 
	glDisable(GL_CULL_FACE);							// disables not showing back faces of triangles 

	// Set the shader program to use for this draw call
	glUseProgram( shaderProgram );				

	// Set up model matrices
	mat4 cityModelMatrix(1.0f);

	// Set up the view matrix
	// The view matrix defines where the viewer is looking
	// Initially fixed, but will be replaced in the tutorial.
	mat4 constantViewMatrix =  mat4(0.707106769f, -0.408248276f, 1.00000000f, 0.000000000f,
		0.000000000f, 0.816496551f, 1.00000000f, 0.000000000f,
		-0.707106769f, -0.408248276f, 1.00000000f, 0.000000000f,
		0.000000000f, 0.000000000f, -30.0000000f, 1.00000000f);

	cameraRight = normalize(cross(cameraDirection, worldUp));
	cameraUp = normalize(cross(cameraRight, cameraDirection));
	mat3 cameraBaseVectorsWorldSpace(cameraRight, cameraUp, -cameraDirection);

	mat4 cameraRotation = mat4(transpose(cameraBaseVectorsWorldSpace));
	mat4 viewMatrix = cameraRotation * translate(-cameraPosition);;

	// Setup the projection matrix
        if (w != old_w || h != old_h)
        {
            pp.h = h;
            pp.w = w;
            old_w = w;
            old_h = h;
        }
	mat4 projectionMatrix = perspective(radians(pp.fov), float(pp.w) / float(pp.h), pp.near, pp.far);
	
	// Concatenate the three matrices and pass the final transform to the vertex shader
	
	// Ground
	mat4 modelViewProjectionMatrix = projectionMatrix * viewMatrix * cityModelMatrix;
	int loc = glGetUniformLocation(shaderProgram, "modelViewProjectionMatrix");
	glUniformMatrix4fv(loc, 1, false, &modelViewProjectionMatrix[0].x);
	render(cityModel);

	// car
	modelViewProjectionMatrix =  projectionMatrix * viewMatrix * carModelMatrix;
	glUniformMatrix4fv(loc, 1, false, &modelViewProjectionMatrix[0].x);
	render(carModel);
	modelViewProjectionMatrix = projectionMatrix * viewMatrix * carModelMatrix2;
	glUniformMatrix4fv(loc, 1, false, &modelViewProjectionMatrix[0].x);
	render(carModel2);


	glUseProgram( 0 );	
}

void gui() {
	// Inform imgui of new frame
	ImGui_ImplSdlGL3_NewFrame(g_window);

        // ----------------- Set variables --------------------------
        ImGui::SliderFloat("Field Of View", &pp.fov, 1.0f, 180.0f, "%.0f");
        ImGui::SliderInt("Width", &pp.w, 256, 1920);
        ImGui::SliderInt("Height", &pp.h, 256, 1080);
        ImGui::Text("Aspect Ratio: %.2f", float(pp.w) / float(pp.h));
        ImGui::SliderFloat("Near Plane", &pp.near, 0.1f, 300.0f, "%.1f");
        ImGui::SliderFloat("Far Plane", &pp.far, 0.1f, 300.0f, "%.1f");
        if (ImGui::Button("Reset"))
        {
            pp.fov = 45.0f;
            pp.w = 1280;
            pp.h = 720;
            pp.near = 0.1f;
            pp.far = 300.0f;
        }
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	// ----------------------------------------------------------

	// Render the GUI.
	ImGui::Render();
}


int main(int argc, char *argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Lab 3");

	// Load shader program
	shaderProgram = labhelper::loadShaderProgram("../lab3-camera/simple.vert", "../lab3-camera/simple.frag");

	// Load models
	loadModels();

	// render-loop
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

		// check new events (keyboard among other)
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			// Allow ImGui to capture events.
			ImGui_ImplSdlGL3_ProcessEvent(&event);

			// More info at https://wiki.libsdl.org/SDL_Event
			if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE)) {
				stopRendering = true;
			}

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
		}

		// implement camera controls based on key states
		// check keyboard state (which keys are still pressed)
		const uint8_t *state = SDL_GetKeyboardState(nullptr);

		// implement controls based on key states
		float speed = 0.3f;
		static mat4 T(1.0f), R(1.0f);
		if (state[SDL_SCANCODE_W]) {
			cameraOffset /= cameraMoveSpeed;
		}
		if (state[SDL_SCANCODE_S]) {
			cameraOffset *= cameraMoveSpeed;
			//cameraPosition -= normalize(cameraDirection) * cameraMoveSpeed;
		}
		if (state[SDL_SCANCODE_UP]) {
			T[3] += speed * R[2];
		}
		if (state[SDL_SCANCODE_DOWN]) {
			T[3] -= speed * R[2];
		}
		if (state[SDL_SCANCODE_LEFT]) {
			R[0] -= 0.03f * R[2];
		}
		if (state[SDL_SCANCODE_RIGHT]) {
			R[0] += 0.03f * R[2];
		}
		if (state[SDL_SCANCODE_SPACE]) {
			cameraDirectionOffset = vec3(0.0f, -0.3f, 1.0f);
		}

		R[0] = normalize(R[0]);
		R[2] = vec4(cross(vec3(R[0]), vec3(R[1])), 0.0f);
		carModelMatrix = T * R;
		cameraPosition = vec3(T[3]) + mat3(R) * cameraOffset;
		cameraDirection = normalize(mat3(R) * cameraDirectionOffset);


		//////////////////////////////////// donut-car
		
		float v = 0.9f, dist = 7.0f;
		vec4 t = vec4(dist, 0.0f, 0.0f, 1.0f);	// rotatepoint - car position
		mat4 translateMatrix = translate(vec3(t));
		mat4 rotateMatrix = rotate(-v * currentTime, worldUp);
		carModelMatrix2 = rotateMatrix * translateMatrix;


	}

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;          
}
