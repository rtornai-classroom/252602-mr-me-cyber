#version 400 core

/** A kontrollpont 2D pozícióját kapjuk a VBO-ból. */
/** We receive the 2D control point position from the VBO. */
layout(location = 0) in vec2 position;

/** Vetítési és nézeti mátrixok C++-ból (ugyanazok, mint a tessellation programban). */
/** Projection and view matrices from C++ (same as in the tessellation program). */
uniform mat4 matProjection;
uniform mat4 matModelView;

void main() {
	/** A vetítési mátrixok alkalmazása a 2D pozícióra. */
	/** Apply the projection matrices to the 2D position. */
	gl_Position = matProjection * matModelView * vec4(position, 0.0, 1.0);
}
