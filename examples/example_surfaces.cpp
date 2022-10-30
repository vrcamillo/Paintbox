#include "common.h"

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

bool init() {
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
	return true;
}

void do_frame() {
	RenderState state;
	
	int window_width, window_height;
	glfwGetFramebufferSize(window, &window_width, &window_height);
	state.viewport.w = window_width;
	state.viewport.h = window_height;
	
	mesh_render(mesh, &state);
}
