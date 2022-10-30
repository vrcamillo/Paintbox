#include <stdlib.h>
#include <stdio.h>
#include "GLFW/glfw3.h"

static GLFWwindow* window;

#include "paintbox.h"
using namespace Paintbox;
// Ideas:
// - 2D normal mapping with shadows
// - 2D fluid simulation
// - Vertex animation textures
// - Cloth and pillow simulator
// - Lightning (bloom)
// - Visualize force fields 
// - Noita-like simulation

Mesh* mesh;

void init() {
	Paintbox::initialize();
	
	Vertex vertices[4] = {
		{{-1, -1, 0}, {1, 0, 1, 1}, {0, 0}},
		{{+1, -1, 0}, {1, 0, 1, 1}, {1, 0}},
		{{+1, +1, 0}, {1, 0, 1, 1}, {1, 1}},
		{{-1, +1, 0}, {1, 0, 1, 1}, {0, 1}},
	};
	
	uint32_t indices[6] = {
		0, 1, 2,
		0, 2, 3,
	};
	
	mesh = mesh_create(4, 6, vertices, indices);
}

void do_frame() {
	RenderState state;
	
	int window_width, window_height;
	glfwGetFramebufferSize(window, &window_width, &window_height);
	state.viewport.w = window_width;
	state.viewport.h = window_height;
	
	mesh_render(mesh, &state);
}

void centralize_window(GLFWwindow* window) {
	auto monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);
	
	int window_width, window_height;
	glfwGetWindowSize(window, &window_width, &window_height);
	
	int x = (vidmode->width - window_width) / 2;
	int y = (vidmode->height - window_height) / 2;
	glfwSetWindowPos(window, x, y);
}

int main() {
	if (!glfwInit()) return -1;
	
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	window = glfwCreateWindow(1920 * 3 / 4, 1080 * 3 / 4, "Paintbox", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	
	centralize_window(window);
	glfwMakeContextCurrent(window);
	
	init();
	
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		do_frame();
		
		glfwSwapBuffers(window);
	}
	
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}