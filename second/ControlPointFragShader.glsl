#version 400 core

/** A szín uniformot C++ állítja be rajzolás előtt: piros a pontokhoz, kék a polygonhoz. */
/** The color uniform is set by C++ before drawing: red for points, blue for the polygon. */
uniform vec3 color;

/** Kimeneti pixelszín. */
/** Output pixel color. */
out vec4 fragColor;

void main() {
	/** A megadott szín alkalmazása, teljesen átlátszatlan (alpha = 1.0). */
	/** Apply the given color, fully opaque (alpha = 1.0). */
	fragColor = vec4(color, 1.0);
}
