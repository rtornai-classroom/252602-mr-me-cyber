#version 400 core

/** Kimeneti pixelszín. */
/** Output pixel color. */
out vec4 fragColor;

void main() {
	/** A görbe minden pixele zöld, teljesen átlátszatlan. */
	/** Every pixel of the curve is green, fully opaque. */
	fragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
