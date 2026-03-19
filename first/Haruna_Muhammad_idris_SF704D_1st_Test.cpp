// Haruna_Muhammad_idris_SF704D_1st_Test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//global states

float circleX = 0.0f, circleY = 0.0f;   // Circle starts at the center of the screen
float circleVx = 0.0f, circleVy = 0.0f; // Velocity — zero until 'S' is pressed
bool  isMoving = false;                  // Tracks whether the animation has started
float speed = 0.008f;                 // Movement speed in NDC units per frame

// The circle radius in NDC units (~60px on a 700px window → 60/350 ≈ 0.171)
const float RADIUS = 60.0f / 350.0f;

float lineY = 0.0f;                      // Y position of the blue horizontal line

// A screen in OpenGL goes from -1.0 to 1.0 (total width 2.0).
// 1/4 of 2.0 is 0.5. A line from -0.25 to +0.25 is exactly 1/4 screen length.
const float LINE_HALF_LENGTH = 0.25f;



// 2. HELPER FUNCTIONS


// Reads a GLSL shader file from disk and returns its source as a string.
// Called once at startup before the shaders are compiled.
std::string readShaderFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "CRITICAL ERROR: Could not open shader file: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Compiles raw GLSL text into executable GPU instructions.
// Returns the compiled shader's ID handle so it can be linked into a program.
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "SHADER COMPILATION FAILED:\n" << infoLog << std::endl;
    }
    return shader;
}

// Keeps a value inside [minVal, maxVal]. Used in the intersection math
// to find the closest point on the finite line segment to the circle center.
float clamp(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}



// 3. KEYBOARD INPUT LOGIC
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {

        // 'S' — start the animation from the center at a 35-degree diagonal angle.
        // Once moving, pressing S again has no effect (guarded by !isMoving).
        if (key == GLFW_KEY_S && !isMoving) {
            isMoving = true;
            float angleRad = 35.0f * (float(M_PI) / 180.0f); // degrees → radians
            circleVx = speed * cos(angleRad); // horizontal speed component
            circleVy = speed * sin(angleRad); // vertical speed component
        }

        // Arrow keys — move the blue horizontal line up or down by a small step.
        if (key == GLFW_KEY_UP)   lineY += 0.02f;
        if (key == GLFW_KEY_DOWN) lineY -= 0.02f;

        // Clamp the line so it cannot be moved off-screen.
        if (lineY > 1.0f) lineY = 1.0f;
        if (lineY < -1.0f) lineY = -1.0f;

        // ESC — close the window and exit the program.
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
    }
}


// ==========================================
// 4. MAIN PROGRAM EXECUTABLE
// ==========================================
int main() {

    // --- Window Setup ---
    if (!glfwInit()) return -1;

    // Request OpenGL 3.3 Core Profile — modern API only, no legacy functions.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Lock window size to exactly 700x700

    // Exactly 700x700 yellow window as required by the assignment.
    GLFWwindow* window = glfwCreateWindow(700, 700, "Bouncing Circle", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);              // V-Sync: cap framerate at the monitor refresh rate
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;


    // --- Shader Compilation ---
    // Read the GLSL source files from disk, compile them, then link into one program.
    std::string vCode = readShaderFile("vertexShader.glsl");
    std::string fCode = readShaderFile("fragmentShader.glsl");
    GLuint vShader = compileShader(GL_VERTEX_SHADER, vCode.c_str());
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fCode.c_str());

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader);
    glAttachShader(shaderProgram, fShader);
    glLinkProgram(shaderProgram);

    // Retrieve the IDs of our Uniforms so C++ can update them each frame.
    // 'offset'    → moves the shape to its current position on screen.
    // 'swapColor' → flips the circle's red/green colors on intersection.
    GLuint offsetLoc = glGetUniformLocation(shaderProgram, "offset");
    GLuint swapColorLoc = glGetUniformLocation(shaderProgram, "swapColor");


    // --- Geometry Data Generation ---
    // We build the circle as a GL_TRIANGLE_FAN:
    //   Vertex 0: center (0, 0) — colored RED
    //   Vertices 1..segments+1: ring points (cos θ, sin θ) — colored GREEN
    //   The last ring point repeats vertex 1 to close the fan.
    // OpenGL interpolates the red-to-green gradient between the center and the ring.

    std::vector<float> circlePositions; // VBO 0: (x, y) per vertex
    std::vector<float> circleColors;    // VBO 1: (R, G, B) per vertex
    int segments = 64;                  // More segments = smoother circle

    // Center vertex — Red
    circlePositions.push_back(0.0f); circlePositions.push_back(0.0f);
    circleColors.push_back(1.0f); circleColors.push_back(0.0f); circleColors.push_back(0.0f);

    // Ring vertices — Green
    // We step from 0 to 2*PI in 'segments' steps, placing a point at each angle.
    for (int i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * float(M_PI) / segments;
        circlePositions.push_back(RADIUS * cos(angle)); // x = r * cos(angle)
        circlePositions.push_back(RADIUS * sin(angle)); // y = r * sin(angle)

        circleColors.push_back(0.0f); // R = 0
        circleColors.push_back(1.0f); // G = 1  → pure green
        circleColors.push_back(0.0f); // B = 0
    }

    // Line endpoints — Blue
    // The line lives at y=0 in its own local space; the 'offset' uniform moves it vertically.
    float linePositions[] = { -LINE_HALF_LENGTH, 0.0f,   LINE_HALF_LENGTH, 0.0f };
    float lineColors[] = { 0.0f, 0.0f, 1.0f,         0.0f, 0.0f, 1.0f };


    // --- GPU Memory Setup (VAO & VBO) ---
    // VAO — remembers the layout of the VBOs so we do not have to re-describe them each frame.
    // VBO — raw memory on the GPU that stores vertex positions and colors.

    // 1. Set up the Circle
    // Two VBOs as required: one for positions (slot 0), one for colors (slot 1).
    GLuint circleVAO, circlePosVBO, circleColVBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circlePosVBO);
    glGenBuffers(1, &circleColVBO);

    glBindVertexArray(circleVAO); // Start recording layout into this VAO

    // Position Buffer → attribute slot 0, 2 floats (x, y) per vertex
    glBindBuffer(GL_ARRAY_BUFFER, circlePosVBO);
    glBufferData(GL_ARRAY_BUFFER, circlePositions.size() * sizeof(float), circlePositions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Color Buffer → attribute slot 1, 3 floats (R, G, B) per vertex
    glBindBuffer(GL_ARRAY_BUFFER, circleColVBO);
    glBufferData(GL_ARRAY_BUFFER, circleColors.size() * sizeof(float), circleColors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    // 2. Set up the Line
    GLuint lineVAO, linePosVBO, lineColVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &linePosVBO);
    glGenBuffers(1, &lineColVBO);

    glBindVertexArray(lineVAO); // Start recording layout into this VAO

    // Position Buffer → attribute slot 0
    glBindBuffer(GL_ARRAY_BUFFER, linePosVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(linePositions), linePositions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Color Buffer → attribute slot 1
    glBindBuffer(GL_ARRAY_BUFFER, lineColVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineColors), lineColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glLineWidth(4.0f); // Thicker line for visibility


    // --- MAIN GAME LOOP ---
    while (!glfwWindowShouldClose(window)) {

        // --- 1. Physics & Edge Bouncing ---
        if (isMoving) {
            circleX += circleVx;
            circleY += circleVy;

            // Bounce exactly off the edges: we factor in RADIUS so the visible
            // edge of the circle — not the invisible center point — touches the wall.
            if (circleX + RADIUS >= 1.0f) { circleX = 1.0f - RADIUS; circleVx = -circleVx; }
            if (circleX - RADIUS <= -1.0f) { circleX = -1.0f + RADIUS; circleVx = -circleVx; }
            if (circleY + RADIUS >= 1.0f) { circleY = 1.0f - RADIUS; circleVy = -circleVy; }
            if (circleY - RADIUS <= -1.0f) { circleY = -1.0f + RADIUS; circleVy = -circleVy; }
        }

        // --- 2. Intersection Math ---
        // Strategy: find the single closest point on the finite line segment to the circle center,
        // then check if that distance is less than the radius.

        // clamp() pins circleX to the line's horizontal extent [-LINE_HALF_LENGTH, +LINE_HALF_LENGTH].
        // This gives us the nearest x on the segment; the nearest y is always lineY.
        float closestX = clamp(circleX, -LINE_HALF_LENGTH, LINE_HALF_LENGTH);
        float closestY = lineY;

        // Pythagorean distance² between circle center and closest point on the line.
        float distX = circleX - closestX;
        float distY = circleY - closestY;

        // If distance² ≤ radius², the circle overlaps the line → trigger color swap.
        bool isIntersecting = (distX * distX + distY * distY) <= (RADIUS * RADIUS);

        // --- 3. Rendering ---
        // Clear screen to Yellow (R=1, G=1, B=0, A=1)
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Draw the Blue Line
        // offset moves the line to its current vertical position (lineY).
        // swapColor is always false — the line's blue color never changes.
        glBindVertexArray(lineVAO);
        glUniform2f(offsetLoc, 0.0f, lineY);
        glUniform1i(swapColorLoc, false);
        glDrawArrays(GL_LINES, 0, 2);

        // Draw the Circle using GL_TRIANGLE_FAN.
        // offset moves the circle to its current physics position (circleX, circleY).
        // swapColor flips red/green in the vertex shader when intersecting the line.
        glBindVertexArray(circleVAO);
        glUniform2f(offsetLoc, circleX, circleY);
        glUniform1i(swapColorLoc, isIntersecting);
        // segments + 2 = 1 center vertex + (segments + 1) ring vertices
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
