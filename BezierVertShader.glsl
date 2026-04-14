#version 400 core

/** A kontrollpont pozícióját kapjuk a VBO-ból (location = 0). */
/** We receive the control point position from the VBO (location = 0). */
layout(location = 0) in vec2 position;

void main() {
	/** A pozíciót változatlanul átadjuk a tessellation control shadernek. */
	/** We pass the position unchanged to the tessellation control shader. */
	/** z = 0.0 (2D rajz), w = 1.0 (homogén koordináta). */
	/** z = 0.0 (2D drawing), w = 1.0 (homogeneous coordinate). */
	gl_Position = vec4(position, 0.0, 1.0);
}
