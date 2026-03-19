#version 330 core

// ==========================================
// FRAGMENT SHADER — Runs once per pixel
// Decides the final COLOR of every pixel
// that OpenGL fills inside a triangle/line
// ==========================================

// --- INPUT FROM VERTEX SHADER ---
// This MUST perfectly match the 'out vec3 vColor' declared in vertexShader.glsl.
// OpenGL automatically interpolates this value across the surface of each triangle,
// which is exactly what produces the smooth red-to-green gradient on the circle.
in vec3 vColor;

// --- OUTPUT TO SCREEN ---
// FragColor is the final RGBA color written to the pixel on screen.
out vec4 FragColor;

void main()
{
    // Output the color passed down from the Vertex Shader.
    // The 1.0 at the end is the Alpha (opacity) channel — fully solid, no transparency.
    FragColor = vec4(vColor, 1.0);
}
