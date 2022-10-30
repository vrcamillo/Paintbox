#pragma once

#include <inttypes.h> // For sized types, like uint32_t, int64_t etc.

// #temporary: The library should allow the user to override the assert and log functions.
#include <stdlib.h> // For exit().
#include <stdio.h>  // For printf().
#define paintbox_log(msg, ...) printf(msg "\n", __VA_ARGS__);
#define paintbox_assert(condition) if (!(condition)) { printf("%s:%d: Assertion failed: \n\t" #condition "\n\n", __FILE__, __LINE__); exit(-1); }
#define paintbox_assert_log(condition, msg, ...) if (!(condition)) { printf("%s:%d: \n", __FILE__, __LINE__); printf("\t" msg "\n\n", __VA_ARGS__); exit(-1); }

// Call this macro before creating a resource to have debug information about it.
#define MARK_NEXT_RESOURCE(name) Paintbox::mark_next_resource(name, __FILE__, __LINE__);

// Example:
// MARK_NEXT_RESOURCE("My Shader");
// Shader* my_shader = shader_create(...);
// printf("id: %lld, name: \"%s\", location: %s:%d\n", my_shader->uid, my_shader->name, my_shader->file, my_shader->line);

namespace Paintbox {

	//
	// Math types
	//
	
	struct vec2 {
		float x;
		float y;
		
		constexpr vec2() : x(0), y(0) {}
		constexpr vec2(float x, float y) : x(x), y(y) {}
	};
	
	union vec3 {
		struct {
			float x;
			float y;
			float z;
		};
		vec2 xy;
		
		constexpr vec3() : x(0), y(0), z(0) {}
		constexpr vec3(float x, float y, float z) : x(x), y(y), z(z) {}
		constexpr vec3(vec2 xy, float z) : x(xy.x), y(xy.y), z(z) {}
	};
	
	union vec4 {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		vec3 xyz;
		vec2 xy;
		
		constexpr vec4() : x(0), y(0), z(0), w(0) {}
		constexpr vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		constexpr vec4(vec2 xy, float z, float w) : x(xy.x), y(xy.y), z(z), w(w) {}
		constexpr vec4(vec3 xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
	};	
	
	union Rect {
		struct {
			float x;
			float y;
			float w;
			float h;
		};
		struct {
			vec2 position;
			vec2 size;
		};
		
		constexpr Rect() : x(0), y(0), w(0), h(0) {}
		constexpr Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
		constexpr Rect(vec2 position, float w, float h) : x(position.x), y(position.y), w(w), h(h) {}
		constexpr Rect(float x, float y, vec2 size) : x(x), y(y), w(size.x), h(size.y) {}
		constexpr Rect(vec2 position, vec2 size) : x(position.x), y(position.y), w(size.x), h(size.y) {}
	};
	
	struct mat4 {
		float m[4][4];
	};
	
	//
	// Graphics stuff
	//
	
	// Since we only use OpenGL for now, there is just one supported shading language. 
	// We probably don't want users to write the same shader many times in different languages, one for each graphics backend.
	// We might want to do something smarter with our shaders, but we'll cross that bridge when we come to it.
	enum class ShaderLanguage {
		GLSL,
		
		COUNT
	};
	
	enum class ShaderType {
		VERTEX,
		PIXEL,
		// In the future, there will be more supported shader types.
		
		COUNT
	};
	
	enum class TextureFormat {
		NONE,
		
		RGBA_U8,
		RGBA_F16,
		
		ALPHA_F32,
		
		COUNT
	};
	
	enum class VertexFormat {
		XYZ_RGBA_UV,
		// In the future, we may support other vertex formats, but for now, this is the default one.
		
		COUNT
	};
	
	struct Resource {
		// Right now, this structure is just used to provide helpful debug information to the user.
		// In the future we might #if out these members in release builds.
		
		uint64_t uid = 0;
		
		const char* name = 0;
		const char* file = 0;
		int32_t line = 0;
	};
	
	struct Shader : Resource {
		ShaderType type {};
	};
	
	struct Texture : Resource {
		TextureFormat format {};
		int32_t width = 0;
		int32_t height = 0;
	};
	
	struct Canvas : Resource {
		TextureFormat format {};
		int32_t width = 0;
		int32_t height = 0;
	};
	
	struct Mesh : Resource {
		int32_t vertex_count = 0;
		int32_t index_count = 0;
	};
	
	union Vertex { // In the future, this will probably be renamed since we will have more vertex formats.
		struct {
			vec3 position;
			vec4 color;
			vec2 uv;
		};
		struct {
			float x;
			float y;
			float z;
			
			float r;
			float g;
			float b;
			float a;
			
			float u;
			float v;
		};
		
		constexpr Vertex() : x(0), y(0), z(0), r(0), g(0), b(0), a(0), u(0), v(0) {}
		constexpr Vertex(vec3 position, vec4 color, vec2 uv) : position(position), color(color), uv(uv) {}
		constexpr Vertex(float x, float y, float z, float r, float g, float b, float a, float u, float v) : x(x), y(y), z(z), r(r), g(g), b(b), a(a), u(u), v(v) {}
	};
	
	static_assert(sizeof(Vertex) == 9 * sizeof(float), "Wrong vertex size!");
	
	struct RenderState {
		Shader* vertex_shader = nullptr; // Null means the default (identity) vertex shader.
		Shader* pixel_shader = nullptr; // Null means the default pixel shader.
		Canvas* canvas = nullptr; // Null means the backbuffer.
		
		Texture* texture0 = nullptr;
		
		Rect viewport = {0, 0, 0, 0};
		
		// Shader constants
		mat4 projection = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1,
		};
	};
	
	//
	// API
	//
	void initialize();
	
	// Shader
	Shader* shader_create(ShaderLanguage language, ShaderType type, const char* shader_source_code);
	// #todo: shader_hotload
	// #todo: shader_destroy
	
	// Texture 
	// Texture* texture_create(TextureFormat format, uint32_t width, uint32_t height, void* image_data);
	// #todo: texture_destroy
	
	// Mesh
	Mesh* mesh_create(uint32_t vertex_count, uint32_t index_count, Vertex vertices[] = nullptr, uint32_t indices[] = nullptr); // If you leave vertices and indices null, this function will just allocate VRAM for the geometry. If that's the case, you must upload mesh data using mesh_upload.
	
	void mesh_upload(Mesh* mesh, uint32_t vertex_count, Vertex vertices[], uint32_t index_count, uint32_t indices[]);
	void mesh_render(Mesh* mesh, RenderState* state, int32_t index_count = -1); // Leave index count as -1 to render all the indices.
		
	// Math functions 
	
	// Operator overloads for math types
	// #todo: These could be generated by a metaprogram, or turned into templates (?)
	vec2 operator-(vec2 v);
	
	vec2 operator+(vec2 a, vec2 b);
	vec2 operator-(vec2 a, vec2 b);
	vec2 operator*(vec2 a, vec2 b);
	vec2 operator/(vec2 a, vec2 b);
	
	vec2 operator*(vec2 v, float f);
	vec2 operator*(float f, vec2 v);
	
	vec2 operator/(vec2 v, float f);
	
	void operator+=(vec2& a, vec2 b);
	void operator-=(vec2& a, vec2 b);
	void operator*=(vec2& a, vec2 b);
	void operator/=(vec2& a, vec2 b);
	
	void operator*=(vec2& v, float f);
	void operator/=(vec2& v, float f);
	
	// vec3
	vec3 operator+(vec3 a, vec3 b);
	vec3 operator-(vec3 a, vec3 b);
	vec3 operator*(vec3 a, vec3 b);
	vec3 operator/(vec3 a, vec3 b);
	
	vec3 operator*(vec3 v, float f);
	vec3 operator*(float f, vec3 v);
	
	vec3 operator/(vec3 v, float f);
	
	void operator+=(vec3& a, vec3 b);
	void operator-=(vec3& a, vec3 b);
	void operator*=(vec3& a, vec3 b);
	void operator/=(vec3& a, vec3 b);
	
	void operator*=(vec3& v, float f);
	void operator/=(vec3& v, float f);
	
	// vec4
	vec4 operator+(vec4 a, vec4 b);
	vec4 operator-(vec4 a, vec4 b);
	vec4 operator*(vec4 a, vec4 b);
	vec4 operator/(vec4 a, vec4 b);
	
	vec4 operator*(vec4 v, float f);
	vec4 operator*(float f, vec4 v);
	
	vec4 operator/(vec4 v, float f);
	
	void operator+=(vec4& a, vec4 b);
	void operator-=(vec4& a, vec4 b);
	void operator*=(vec4& a, vec4 b);
	void operator/=(vec4& a, vec4 b);
	
	void operator*=(vec4& v, float f);
	void operator/=(vec4& v, float f);
	
	// mat4
	vec4 operator*(mat4 m, vec4 v);
	mat4 operator*(mat4 a, mat4 b);
	
	// Debugging functions
	void mark_next_resource(const char* name, const char* file, int32_t line);
	void register_resource(Resource* resource);
	
}