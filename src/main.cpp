#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Material.h"
#include "Light.h"

using namespace std;

GLFWwindow* window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Shape> bunny, teapot;
shared_ptr<Program> prog;
vector<shared_ptr<Program>> progs;
int pid = 1;

// materials
int mat = 0; // selected material
vector<Material> materials;

// lights
vector<Light> lights;
int curr_light = 0; // current selected light

// cel levels
int celLevels = 4;

bool keyToggles[256] = { false }; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char* description) {
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if (action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse) {
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if (state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

static void char_callback(GLFWwindow* window, unsigned int key) {
	keyToggles[key] = !keyToggles[key];
	switch (key) {
	case 'm': // material
		if (mat < materials.size() - 1) mat++;
		break;
	case 'M':
		if (mat > 0) mat--;
		break;
	case 's': // shader
		if (pid < progs.size() - 1) pid++;
		break;
	case 'S':
		if (pid > 0) pid--;
		break;
	case 'l': // light selector
		if (curr_light < 2) curr_light++;
		break;
	case 'L':
		if (curr_light > 0) curr_light--;
		break;
	case 'x': // light x
		lights.at(curr_light).pos.x -= 0.5f;
		break;
	case 'X':
		lights.at(curr_light).pos.x += 0.5f;
		break;
	case 'y': // light y
		lights.at(curr_light).pos.y -= 0.5f;
		break;
	case 'Y':
		lights.at(curr_light).pos.y += 0.5f;
		break;
	case 'j': // cel shading levels
		celLevels++;
		break;
	case 'J':
		if (celLevels > 1) celLevels--;
		break;
	}
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char* filepath, GLFWwindow* w) {
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if (rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
	}
}

// This function is called once to initialize the scene and OpenGL
static void init() {
	// Initialize time.
	glfwSetTime(0.0);

	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	// Set up programs
	progs.push_back(make_shared<Program>()); // normal shader
	progs.back()->setShaderNames(RESOURCE_DIR + "normal_vert.glsl", RESOURCE_DIR + "normal_frag.glsl");
	progs.push_back(make_shared<Program>()); // bp shader
	progs.back()->setShaderNames(RESOURCE_DIR + "bp_vert.glsl", RESOURCE_DIR + "bp_frag.glsl");
	progs.push_back(make_shared<Program>()); // silhouette shader
	progs.back()->setShaderNames(RESOURCE_DIR + "bp_vert.glsl", RESOURCE_DIR + "sil_frag.glsl");
	progs.push_back(make_shared<Program>()); // cel shader
	progs.back()->setShaderNames(RESOURCE_DIR + "bp_vert.glsl", RESOURCE_DIR + "cel_frag.glsl");
	for (int i = 0; i < progs.size(); i++) {
		progs.at(i)->setVerbose(true);
		progs.at(i)->init();
		progs.at(i)->addAttribute("aPos");
		progs.at(i)->addAttribute("aNor");
		progs.at(i)->addUniform("MV");
		progs.at(i)->addUniform("IT");
		progs.at(i)->addUniform("P");
		progs.at(i)->addUniform("ka");
		progs.at(i)->addUniform("kd");
		progs.at(i)->addUniform("ks");
		progs.at(i)->addUniform("s");
		progs.at(i)->addUniform("lightPos1");
		progs.at(i)->addUniform("lightColor1");
		progs.at(i)->addUniform("lightPos2");
		progs.at(i)->addUniform("lightColor2");
		progs.at(i)->addUniform("celLevels");
		progs.at(i)->setVerbose(false);
	}

	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation

	bunny = make_shared<Shape>();
	bunny->loadMesh(RESOURCE_DIR + "bunny.obj");
	bunny->init();
	teapot = make_shared<Shape>();
	teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
	teapot->init();

	GLSL::checkError(GET_FILE_LINE);

	// create materials
	materials.push_back(Material({ 0.2f, 0.2f, 0.2f }, { 0.8f, 0.7f, 0.7f }, { 1.0f, 0.9f, 0.8f }, 200.0f));
	materials.push_back(Material({ 0.2f, 0.2f, 0.2f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.8f, 0.4f }, 200.0f));
	materials.push_back(Material({ 0.2f, 0.2f, 0.2f }, { 0.35f, 0.38f, 0.56f }, { 0.02f, 0.02f, 0.02f }, 200.0f));

	// create lights
	lights.push_back(Light({ 1.0f, 1.0f, 1.0f }, { 0.8f, 0.8f, 0.8f }));
	lights.push_back(Light({ -1.0f, 1.0f, 1.0f }, { 0.2f, 0.2f, 0.0f }));
}

// This function is called every frame to draw the scene.
static void render() {
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if (keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width / (float)height);

	double t = glfwGetTime();
	if (!keyToggles[(unsigned)' ']) {
		// Spacebar turns animation on/off
		t = 0.0f;
	}

	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();

	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);

	progs.at(pid)->bind();
	// set variables
	glUniform3f(progs.at(pid)->getUniform("ka"), materials.at(mat).ka.x, materials.at(mat).ka.y, materials.at(mat).ka.z);
	glUniform3f(progs.at(pid)->getUniform("kd"), materials.at(mat).kd.x, materials.at(mat).kd.y, materials.at(mat).kd.z);
	glUniform3f(progs.at(pid)->getUniform("ks"), materials.at(mat).ks.x, materials.at(mat).ks.y, materials.at(mat).ks.z);
	glUniform1f(progs.at(pid)->getUniform("s"), materials.at(mat).s);
	glUniform3f(progs.at(pid)->getUniform("lightPos1"), lights.at(0).pos.x, lights.at(0).pos.y, lights.at(0).pos.z);
	glUniform3f(progs.at(pid)->getUniform("lightPos2"), lights.at(1).pos.x, lights.at(1).pos.y, lights.at(1).pos.z);
	glUniform3f(progs.at(pid)->getUniform("lightColor1"), lights.at(0).color.x, lights.at(0).color.y, lights.at(0).color.z);
	glUniform3f(progs.at(pid)->getUniform("lightColor2"), lights.at(1).color.x, lights.at(1).color.y, lights.at(1).color.z);
	glUniform1i(progs.at(pid)->getUniform("celLevels"), celLevels);
	glUniformMatrix4fv(progs.at(pid)->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	// Apply model transforms and draw
	// bunny
	MV->pushMatrix();
	MV->translate(-0.5f, -0.5f, 0.0f);
	MV->scale(0.5f);
	MV->rotate((float)t, 0.0f, 1.0f, 0.0f);

	glUniformMatrix4fv(progs.at(pid)->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glm::mat4 it = glm::transpose(glm::inverse(MV->topMatrix()));
	glUniformMatrix4fv(progs.at(pid)->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(it));
	bunny->draw(progs.at(pid));
	MV->popMatrix();
	// teapot
	MV->pushMatrix();
	MV->translate(0.5f, 0.0f, 0.0f);
	MV->scale(0.5f);
	glm::mat4 S(1.0f); // shear
	S[0][1] = 0.5f * cos((float)t);
	MV->multMatrix(S);
	MV->rotate((float)M_PI, 0.0f, 1.0f, 0.0f);

	glUniformMatrix4fv(progs.at(pid)->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	it = glm::transpose(glm::inverse(MV->topMatrix()));
	glUniformMatrix4fv(progs.at(pid)->getUniform("IT"), 1, GL_FALSE, glm::value_ptr(it));
	teapot->draw(progs.at(pid));
	MV->popMatrix();

	progs.at(pid)->unbind();

	MV->popMatrix();
	P->popMatrix();

	GLSL::checkError(GET_FILE_LINE);

	if (OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char** argv) {
	if (argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");

	// Optional argument
	if (argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if (!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
