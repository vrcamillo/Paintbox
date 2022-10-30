#include "paintbox.h"

#include <stddef.h> // For offsetof
#include "glad/gl.h"
#include "GLFW/glfw3.h" // #temporary

static const char* glsl_default_vertex_shader_source = R"glsl(
#version 410

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec4 vertex_color;
layout (location = 2) in vec2 vertex_uv;

uniform mat4 projection;

out vec2 pixel_uv;
out vec4 pixel_color;

void main() {
	vec4 world_position = vec4(vertex_position, 1);
	gl_Position = projection * world_position;
	pixel_color = vertex_color;
	pixel_uv = vertex_uv;
}  
)glsl";

static const char* glsl_default_pixel_shader_source = R"glsl(
#version 410

in vec4 pixel_color;
in vec2 pixel_uv;
out vec4 result_color; 

void main() {
	result_color = pixel_color;
}

)glsl";

namespace Paintbox {
	
	struct ShaderGL : Shader {
		GLuint handle = 0; // OpenGL shader handle.
	};
	
	struct TextureGL : Texture {
		GLuint handle = 0; // OpenGL texture handle.
	};
	
	struct CanvasGL : Canvas {
		GLuint fbo = 0; // OpenGL Framebuffer buffer object.
		Texture* color_attachment = nullptr;
	};
	
	struct MeshGL : Mesh {
		GLuint vbo = 0; // OpenGL Vertex buffer object.
		GLuint ibo = 0; // OpenGL Index buffer object.
	};
	
	struct ShaderLinkage {
		GLuint program = 0;
		Shader* vertex_shader = 0;
		Shader* pixel_shader = 0;
	};
	
	static GLuint vertex_array_objects[VertexFormat::COUNT];
	
	static Shader* default_vertex_shader;
	static Shader* default_pixel_shader;
	
	// #temporary: Eventually we will want an actual table here.
	constexpr int shader_linkage_table_capacity = 128;
	static int shader_linkage_table_length;
	static ShaderLinkage shader_linkage_table[shader_linkage_table_capacity]; 
	
	static bool backend_initialized;
	
	void initialize() {
		gladLoadGL(glfwGetProcAddress); // #temporary: There should not be a glfw dependency here.
		
		{
			//
			// Create our vertex array objects.
			// 
			
			static_assert((int) VertexFormat::COUNT == 1, "Please implement VAO bindings for any new vertex format.");
			glGenVertexArrays((int) VertexFormat::COUNT, vertex_array_objects);
			
			glBindVertexArray(vertex_array_objects[(int) VertexFormat::XYZ_RGBA_UV]);
			
			// Vertex position (xyz)
			glEnableVertexAttribArray(0);
			glVertexAttribBinding(0, 0);
			glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
			
			// Vertex color (rgba)
			glEnableVertexAttribArray(1);
			glVertexAttribBinding(1, 0);
			glVertexAttribFormat(1, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
			
			// Vertex uv (uv)
			glEnableVertexAttribArray(2);
			glVertexAttribBinding(2, 0);
			glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
			
			glBindVertexArray(0);
		}
		
		{
			//
			// Create our default shaders
			// 
			
			MARK_NEXT_RESOURCE("Default Vertex");
			default_vertex_shader = shader_create(ShaderLanguage::GLSL, ShaderType::VERTEX, glsl_default_vertex_shader_source);
			paintbox_assert(default_vertex_shader);
			
			MARK_NEXT_RESOURCE("Default Pixel");
			default_pixel_shader = shader_create(ShaderLanguage::GLSL, ShaderType::PIXEL, glsl_default_pixel_shader_source);
			paintbox_assert(default_pixel_shader);
		}
		
		
		backend_initialized = true;	
	}
	
	Shader* shader_create(ShaderLanguage language, ShaderType type, const char* shader_source_code) {
		GLenum gl_shader_type = -1;
		switch (type) {
		  case ShaderType::VERTEX: gl_shader_type = GL_VERTEX_SHADER;   break;
		  case ShaderType::PIXEL:  gl_shader_type = GL_FRAGMENT_SHADER; break;
		  default: paintbox_assert(false);
		}
		
		GLuint handle = glCreateShader(gl_shader_type);
		glShaderSource(handle, 1, &shader_source_code, nullptr);
		glCompileShader(handle);
		
		int shader_compiled;
		glGetShaderiv(handle, GL_COMPILE_STATUS, &shader_compiled);
		if (!shader_compiled) {
			char message[512];
			glGetShaderInfoLog(handle, sizeof(message), nullptr, message);
			paintbox_log("Failed to compile shader:\n%s", message);
			glDeleteShader(handle);
			return nullptr;
		}
		
		ShaderGL* result = new ShaderGL; // #memory_cleanup
		register_resource(result);
		result->type = type;
		result->handle = handle;
		return result;
	}
	
	static ShaderLinkage* gl_get_or_create_shader_linkage(Shader* vertex_shader, Shader* pixel_shader) {		
		if (!vertex_shader) vertex_shader = default_vertex_shader;
		if (!pixel_shader)  pixel_shader = default_pixel_shader;
		
		// #temporary: This should be a table lookup.
		for (int i = 0; i < shader_linkage_table_length; i += 1) {
			ShaderLinkage* entry = &shader_linkage_table[i];
			if (vertex_shader == entry->vertex_shader && pixel_shader == entry->pixel_shader) {
				return entry;
			}
		}	
		
		paintbox_assert(shader_linkage_table_length < shader_linkage_table_capacity); // #robustness
		
		// If we get here, it means we did not find both shaders linked together in a program, so we generate a new one.
		GLuint program = glCreateProgram();
		glAttachShader(program, ((ShaderGL*) vertex_shader)->handle);
		glAttachShader(program, ((ShaderGL*) pixel_shader)->handle);
		glLinkProgram(program);
		
		int program_linked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
		if (!program_linked) {
			char message[512];
			glGetProgramInfoLog(program, sizeof(message), nullptr, message);
			paintbox_log("Failed to link vertex shader '%s' and pixel shader '%s':\n%s", vertex_shader->name, pixel_shader->name, message);
			glDeleteProgram(program);
			return nullptr;
		}
		
		auto entry = &shader_linkage_table[shader_linkage_table_length++];
		entry->program = program;
		entry->vertex_shader = vertex_shader;
		entry->pixel_shader = pixel_shader;
		return entry;
	}
	
	Mesh* mesh_create(uint32_t vertex_count, uint32_t index_count, Vertex vertices[], uint32_t indices[]) { 
		uint32_t vertex_buffer_size = vertex_count * sizeof(vertices[0]);
		uint32_t index_buffer_size = index_count * sizeof(indices[0]);
		
		// The rationale here is that, if we provide vertices at mesh_create, this mesh is probably going to be static throughout the program; otherwise, we assume it will be updated regularly.
		// This doesn't have to be true, and OpenGL guaranteees (https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferData.xhtml) that these are just hints that are only used for performance optimizations within the driver.
		GLenum usage = (vertices == nullptr) ? GL_STREAM_DRAW : GL_STATIC_DRAW;
		
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, vertices, usage);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		GLuint ibo;
		glGenBuffers(1, &ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_size, indices, usage);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
		MeshGL* result = new MeshGL; // #memory_cleanup
		register_resource(result);
		result->vbo = vbo;
		result->ibo = ibo;
		result->vertex_count = vertex_count;
		result->index_count = index_count;
		return result;
	}	
	
	void mesh_upload(Mesh* mesh, uint32_t vertex_count, Vertex vertices[], uint32_t index_count, uint32_t indices[]) {
		// #speed: OpenGL syncs internally. This function can take up too much time.
		// Eventually we should be smarter about memory uploads to the GPU.
		
		auto mesh_gl = (MeshGL*) mesh;
		
		paintbox_assert(vertex_count <= mesh_gl->vertex_count);
		paintbox_assert(index_count <= mesh_gl->index_count);
		
		int32_t vertex_buffer_size = vertex_count * sizeof(vertices[0]);
		int32_t index_buffer_size = index_count * sizeof(indices[0]);
		
		glBindBuffer(GL_ARRAY_BUFFER, mesh_gl->vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_buffer_size, vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_gl->ibo);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, index_buffer_size, indices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void mesh_render(Mesh* mesh, RenderState* state, int32_t index_count) {
		auto mesh_gl = (MeshGL*) mesh;
		
		if (index_count < 0) index_count = mesh_gl->index_count;
		
		auto linkage = gl_get_or_create_shader_linkage(state->vertex_shader, state->pixel_shader);
		paintbox_assert(linkage);
		glUseProgram(linkage->program);
		
		Rect viewport = state->viewport;
		glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
		
		if (state->texture0) {
			auto texture0_gl = (TextureGL*) state->texture0;
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture0_gl->handle);
		}
		
		if (state->texture1) {
			auto texture1_gl = (TextureGL*) state->texture1;
			
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, texture1_gl->handle);
		}
		
		
		// Apply uniforms
		GLuint projection_uniform_loc = glGetUniformLocation(linkage->program, "projection");
		if (projection_uniform_loc >= 0) {
			glUniformMatrix4fv(projection_uniform_loc, 1, GL_TRUE, (float*) &state->projection);
		} else {
			// #incomplete #robustness: Provide a helpful error message here.
		}
		
		GLuint texture0_uniform_loc = glGetUniformLocation(linkage->program, "texture0");
		if (texture0_uniform_loc >= 0) {
			glUniform1i(texture0_uniform_loc, 0);
		} else {
			// #incomplete #robustness: Provide a helpful error message here.
		}
		
		GLuint texture1_uniform_loc = glGetUniformLocation(linkage->program, "texture1");
		if (texture1_uniform_loc >= 0) {
			glUniform1i(texture1_uniform_loc, 1);
		} else {
			// #incomplete #robustness: Provide a helpful error message here.
		}
		
		GLuint time_uniform_loc = glGetUniformLocation(linkage->program, "time");
		if (time_uniform_loc >= 0) {
			glUniform1f(time_uniform_loc, glfwGetTime()); // #temporary
		} else {
			// #incomplete #robustness: Provide a helpful error message here.
		}
		
		glBindVertexArray(vertex_array_objects[(int) VertexFormat::XYZ_RGBA_UV]);
		
		static_assert((int) VertexFormat::COUNT == 1, "We assume all meshes use XYZ_RGBA_UV for now.");
		
		// Bind the vertex format and mesh buffers
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_gl->ibo);
		glBindVertexBuffer(0, mesh_gl->vbo, 0, sizeof(Vertex));
		
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, (void*) 0);
		
		// Maybe these are not necessary, but I'm being paranoid here.
		glBindVertexArray(0);
		glUseProgram(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	
	struct GLTextureFormatInfo {
		GLenum gl_format;
		GLenum gl_internal_format;
		GLenum gl_type;
		GLint gl_swizzle[4];
	};
	
	static GLTextureFormatInfo gl_get_texture_format_info(TextureFormat format) {
		GLTextureFormatInfo info = {};
		
		info.gl_swizzle[0] = GL_RED;
		info.gl_swizzle[1] = GL_GREEN;
		info.gl_swizzle[2] = GL_BLUE;
		info.gl_swizzle[3] = GL_ALPHA;
		
		switch (format) {
		  case TextureFormat::RGBA_U8: {
				info.gl_internal_format = GL_RGBA8; 
				info.gl_type = GL_UNSIGNED_BYTE;
				info.gl_format = GL_RGBA;
			} break;
			
		  case TextureFormat::RGBA_S8: {
				info.gl_internal_format = GL_RGBA8; 
				info.gl_type = GL_BYTE;
				info.gl_format = GL_RGBA;
			} break;
			
			
		  case TextureFormat::RGBA_F16: {
				info.gl_internal_format = GL_RGBA16F; 
				info.gl_type = GL_HALF_FLOAT;
				info.gl_format = GL_RGBA;
			} break;
			
		  case TextureFormat::ALPHA_F32: {
				info.gl_internal_format = GL_R32F; 
				info.gl_type = GL_FLOAT;
				info.gl_format = GL_RED;
				
				info.gl_swizzle[0] = GL_ONE;
				info.gl_swizzle[1] = GL_ONE;
				info.gl_swizzle[2] = GL_ONE;
				info.gl_swizzle[3] = GL_RED;
			} break;
			
		  default: 
			paintbox_assert(false);
		}	
		
		return info;
	}
	
	Texture* texture_create(TextureFormat format, int32_t width, int32_t height, void* image_data) {
		auto format_info = gl_get_texture_format_info(format);
		
		GLuint handle;
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_2D, handle);
		glTexImage2D(GL_TEXTURE_2D, 0, format_info.gl_internal_format, width, height, 0, format_info.gl_format, format_info.gl_type, image_data);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, format_info.gl_swizzle);
		
		glBindTexture(GL_TEXTURE_2D, 0);
		
		TextureGL* texture = new TextureGL; // #memory_cleanup
		register_resource(texture);
		texture->format = format;
		texture->width = width;
		texture->height = height;
		texture->handle = handle;
		return texture;
	}
	
}