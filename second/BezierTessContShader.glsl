#version 400 core

/** A kimenet maximális kontrollpont száma. Meg kell egyeznie a main.cpp MAX_CONTROL_POINTS értékével. */
/** Maximum output control point count. Must match the MAX_CONTROL_POINTS value in main.cpp. */
#define MAX_CONTROL_POINTS 16
layout(vertices = MAX_CONTROL_POINTS) out;

/** A görbe finomítási szintje: hány szegmensből álljon a kirajzolt görbe. */
/** The tessellation level: how many segments the rendered curve shall consist of. */
uniform float tessLevel;

void main() {
	/** Csak érvényes indexű bemeneti pontokat másoljuk ki (a többi undefined lenne). */
	/** Only copy input points with valid indices (the rest would be undefined). */
	/** gl_PatchVerticesIn: a tényleges bemeneti kontrollpontok száma. */
	/** gl_PatchVerticesIn: the actual number of input control points. */
	if (gl_InvocationID < gl_PatchVerticesIn)
		gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	else
		gl_out[gl_InvocationID].gl_Position = vec4(0.0);

	/** Csak az első invokáció állítja be a tessellation szinteket. */
	/** Only the first invocation sets the tessellation levels. */
	/** Isolines esetén: [0] = vonalak száma (1), [1] = szegmensek száma vonalonként. */
	/** For isolines: [0] = number of lines (1), [1] = number of segments per line. */
	if (gl_InvocationID == 0) {
		gl_TessLevelOuter[0] = 1.0;
		gl_TessLevelOuter[1] = tessLevel;
	}
}
