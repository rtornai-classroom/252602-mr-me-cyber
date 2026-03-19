#version 330 core

// ==========================================
// VERTEX SHADER — Runs once per vertex
// Decides WHERE each point goes on screen
// and WHAT COLOR it should carry forward
// ==========================================

// --- INPUTS FROM C++ ---
// 'layout(location = 0)' maps directly to the first VBO in our C++ code (Positions).
// For the circle: (x, y) on a unit-radius circle.
// For the line:   the two endpoint positions.
layout(location = 0) in vec2 aPos;

// 'layout(location = 1)' maps directly to the second VBO in our C++ code (Colors).
// For the circle center: red  (1, 0, 0)
// For the circle edge:   green(0, 1, 0)
// For the line:          blue (0, 0, 1)
layout(location = 1) in vec3 aColor;

// --- UNIFORMS FROM C++ ---
// Uniforms are global variables set by C++ once per draw call.
// 'offset' moves the entire shape to its current world position.
// For the circle it carries (circleX, circleY).
// For the line   it carries (0.0,     lineY  ).
uniform vec2 offset;

// A true/false switch set by C++ when an intersection is detected.
// When true the Red and Green channels of the circle are swapped,
// turning "red center / green border" into "green center / red border".
uniform bool swapColor;

// --- OUTPUT TO FRAGMENT SHADER ---
// The interpolated color travels down the pipeline to every pixel
// that sits between two vertices.  OpenGL blends it automatically,
// giving us the smooth red-to-green gradient across the circle.
out vec3 vColor;

void main()
{
    // gl_Position is a mandatory built-in OpenGL variable.
    // We add the movement offset to the base vertex position
    // so the shape appears at the correct place on screen.
    // z = 0.0 (2D — no depth),  w = 1.0 (standard for non-perspective).
    gl_Position = vec4(aPos + offset, 0.0, 1.0);

    // Swap Red and Green channels if C++ detects an intersection.
    if (swapColor) {
        // aColor is (R, G, B). Rearranging to (G, R, B) flips the gradient:
        // the red center becomes green, the green border becomes red.
        vColor = vec3(aColor.g, aColor.r, aColor.b);
    } else {
        // Normal state: keep the original green border / red center colors.
        vColor = aColor;
    }
}
