#include "paintbox.h"

namespace Paintbox {
	mat4 orthographic(float left, float right, float top, float bottom, float near, float far) {
		float dx = right - left;
		float dy = top - bottom;
		float dz = far - near;
		
		float sx = 2.0f / dx;
		float sy = 2.0f / dy;
		float sz = 2.0f / dz;
		
		float tx = -(right + left) / dx;
		float ty = -(top + bottom) / dy;
		float tz = -(far + near)   / dz;
		
		return {
			sx, 0, 0, tx,
			0, sy, 0, ty,
			0, 0, sz, tz,
			0, 0, 0, 1,
		};
	}
	
}