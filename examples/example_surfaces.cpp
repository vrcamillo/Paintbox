#define EXAMPLE_NAME "Surfaces"
#include "common.h"

#include "paintbox.h"
using namespace Paintbox;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Ideas:
// - 2D normal mapping with shadows
// - 2D fluid simulation
// - Vertex animation textures
// - Cloth and pillow simulator
// - Lightning (bloom)
// - Visualize force fields 
// - Noita-like simulation

Mesh* mesh;
Texture* color_texture;
Texture* normal_texture;

Shader* surface_pixel_shader;

static const char* glsl_surface_pixel_shader_source = R"glsl(
#version 410

in vec4 pixel_color;
in vec2 pixel_uv;
out vec4 result_color; 

uniform float time;

uniform sampler2D texture0;
uniform sampler2D texture1;

void main() {
	vec4 color = texture(texture0, pixel_uv);
	vec3 normal = normalize(2.0 * texture(texture1, pixel_uv).xyz - 1.0);
	
	vec3 light_position = vec3(0.5 + 0.5 * sin(time * 5), 0.5 + 0.5 * cos(time * 5), 1);
	vec3 delta = vec3(pixel_uv, 0) - light_position; 
	
	float factor = 1.0 / dot(delta, delta);
	factor *= clamp(-dot(normalize(delta), normal), 0, 1);
	
	result_color = vec4(color.rgb * factor, color.w);
}

)glsl";

bool init() {
	Paintbox::initialize();
	
	surface_pixel_shader = shader_create(ShaderLanguage::GLSL, ShaderType::PIXEL, glsl_surface_pixel_shader_source);
	
	Vertex vertices[4] = {
		{{-0.5, -0.5, 0}, {1, 0, 1, 1}, {0, 0}},
		{{+0.5, -0.5, 0}, {1, 0, 1, 1}, {1, 0}},
		{{+0.5, +0.5, 0}, {1, 0, 1, 1}, {1, 1}},
		{{-0.5, +0.5, 0}, {1, 0, 1, 1}, {0, 1}},
	};
	
	uint32_t indices[6] = {
		0, 1, 2,
		0, 2, 3,
	};
	
	mesh = mesh_create(4, 6, vertices, indices);
	
	stbi_set_flip_vertically_on_load(1);
	
	int color_width, color_height, color_components;
	auto color_data = stbi_load("../assets/materials/gravel/Gravel033_1K_Color.jpg", &color_width, &color_height, &color_components, 4);
	color_texture = texture_create(TextureFormat::RGBA_U8, color_width, color_height, color_data);
	
	int normal_width, normal_height, normal_components;
	auto normal_data = stbi_load("../assets/materials/gravel/Gravel033_1K_NormalGL.jpg", &normal_width, &normal_height, &normal_components, 4);
	normal_texture = texture_create(TextureFormat::RGBA_U8, normal_width, normal_height, normal_data);
	
	return true;
}

void do_frame() {
	RenderState state;
	
	int window_width, window_height;
	glfwGetFramebufferSize(window, &window_width, &window_height);
	state.viewport.w = window_width;
	state.viewport.h = window_height;
	
	state.texture0 = color_texture;
	state.texture1 = normal_texture;
	state.pixel_shader = surface_pixel_shader;
	state.projection = orthographic(-0.8, 0.8, 0.5, -0.5, -1, +1);
	
	mesh_render(mesh, &state);
}
