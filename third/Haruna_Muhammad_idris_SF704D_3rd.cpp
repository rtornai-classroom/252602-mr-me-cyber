// ============================================================
//  Assignment 3 â€“ OpenGL Cubes, Camera, Lighting, Texture
//
//  CAMERA CONTROLS (mouse):
//    Left-click + drag horizontally  â†’ orbit around Y-axis
//    Left-click + drag vertically    â†’ move camera up/down
//    Scroll wheel                    â†’ zoom in/out (radius)
//
//  KEYBOARD:
//    L  â€“ toggle yellow diffuse light on/off
//    M  â€“ toggle magenta material on/off
//    ESC â€“ quit
// ============================================================

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>

// ----------------------------------------------------------------
//  File / shader helpers
// ----------------------------------------------------------------

static std::string loadFile(const char* path)
{
    std::ifstream f(path);
    if (!f) { std::cerr << "Cannot open: " << path << "\n"; return ""; }
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); std::cerr << "Shader:\n" << log << "\n"; }
    return s;
}

static GLuint buildProgram(const char* vp, const char* fp)
{
    std::string vs = loadFile(vp), fs = loadFile(fp);
    GLuint v = compileShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs.c_str());
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(p, 512, nullptr, log); std::cerr << "Link:\n" << log << "\n"; }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

static GLuint loadTexture(const char* path)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded texture: " << path << "\n";
    }
    else {
        std::cerr << "Failed to load texture: " << path << "\n";
        unsigned char yellow[3] = { 255,220,0 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, yellow);
    }
    stbi_image_free(data);
    return tex;
}

// ----------------------------------------------------------------
//  Geometry
// ----------------------------------------------------------------

static const float cubeVertices[] = {
    // Back
    -0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,
     0.5f, 0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1, -0.5f,-0.5f,-0.5f, 0,0,-1,
     // Front
     -0.5f,-0.5f, 0.5f, 0,0,1,   0.5f,-0.5f, 0.5f, 0,0,1,   0.5f, 0.5f, 0.5f, 0,0,1,
      0.5f, 0.5f, 0.5f, 0,0,1,  -0.5f, 0.5f, 0.5f, 0,0,1,  -0.5f,-0.5f, 0.5f, 0,0,1,
      // Left
      -0.5f, 0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f,-0.5f,-1,0,0,  -0.5f,-0.5f,-0.5f,-1,0,0,
      -0.5f,-0.5f,-0.5f,-1,0,0,  -0.5f,-0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f, 0.5f,-1,0,0,
      // Right
       0.5f, 0.5f, 0.5f, 1,0,0,   0.5f, 0.5f,-0.5f, 1,0,0,   0.5f,-0.5f,-0.5f, 1,0,0,
       0.5f,-0.5f,-0.5f, 1,0,0,   0.5f,-0.5f, 0.5f, 1,0,0,   0.5f, 0.5f, 0.5f, 1,0,0,
       // Bottom
       -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0,
        0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f,-0.5f, 0,-1,0,
        // Top
        -0.5f, 0.5f,-0.5f, 0,1,0,   0.5f, 0.5f,-0.5f, 0,1,0,   0.5f, 0.5f, 0.5f, 0,1,0,
         0.5f, 0.5f, 0.5f, 0,1,0,  -0.5f, 0.5f, 0.5f, 0,1,0,  -0.5f, 0.5f,-0.5f, 0,1,0,
};

static std::vector<float> buildSphere(int stacks, int slices, float radius)
{
    std::vector<float> v;
    const float PI = 3.14159265358979f;
    for (int i = 0; i < stacks; ++i) {
        float p0 = PI * (float(i) / stacks - 0.5f), p1 = PI * (float(i + 1) / stacks - 0.5f);
        for (int j = 0; j < slices; ++j) {
            float t0 = 2 * PI * float(j) / slices, t1 = 2 * PI * float(j + 1) / slices;
            auto push = [&](float phi, float th) {
                v.push_back(radius * cosf(phi) * cosf(th));
                v.push_back(radius * sinf(phi));
                v.push_back(radius * cosf(phi) * sinf(th));
                v.push_back(th / (2 * PI));
                v.push_back((phi + PI * 0.5f) / PI);
                };
            push(p0, t0); push(p1, t0); push(p1, t1);
            push(p0, t0); push(p1, t1); push(p0, t1);
        }
    }
    return v;
}

// ----------------------------------------------------------------
//  Global state
// ----------------------------------------------------------------

bool  lightOn = true;
bool  materialOn = false;

// Camera â€“ cylindrical coordinates
float camAngle = 0.3f;   // horizontal angle (radians)
float camHeight = 2.0f;   // Y position
float camRadius = 7.0f;   // distance from Y-axis

// Mouse drag state
bool  mouseDown = false;
double lastMouseX = 0.0, lastMouseY = 0.0;

// Sensitivity tuning
const float MOUSE_ANGLE_SENS = 0.005f;  // rad per pixel (horizontal drag)
const float MOUSE_HEIGHT_SENS = 0.01f;   // units per pixel (vertical drag)
const float SCROLL_SENS = 0.5f;    // units per scroll tick

// Light animation
float lightAngle = 0.0f;
float lightRadius = 4.0f;

// Key-toggle bookkeeping
bool keyL_prev = false, keyM_prev = false;

// ----------------------------------------------------------------
//  GLFW callbacks
// ----------------------------------------------------------------

void mouseButtonCallback(GLFWwindow* /*win*/, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mouseDown = (action == GLFW_PRESS);
    }
}

void cursorPosCallback(GLFWwindow* /*win*/, double xpos, double ypos)
{
    if (!mouseDown) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        return;
    }

    double dx = xpos - lastMouseX;
    double dy = ypos - lastMouseY;   // positive = mouse moved down

    camAngle += float(dx) * MOUSE_ANGLE_SENS;
    camHeight -= float(dy) * MOUSE_HEIGHT_SENS;  // drag up â†’ camera goes up

    lastMouseX = xpos;
    lastMouseY = ypos;
}

void scrollCallback(GLFWwindow* /*win*/, double /*xoffset*/, double yoffset)
{
    // Scroll up â†’ zoom in (smaller radius), scroll down â†’ zoom out
    camRadius -= float(yoffset) * SCROLL_SENS;
    if (camRadius < 2.0f)  camRadius = 2.0f;
    if (camRadius > 30.0f) camRadius = 30.0f;
}

// ----------------------------------------------------------------
//  Keyboard (only for L / M / ESC, not camera)
// ----------------------------------------------------------------

void processInput(GLFWwindow* win)
{
    bool lNow = (glfwGetKey(win, GLFW_KEY_L) == GLFW_PRESS);
    if (lNow && !keyL_prev) lightOn = !lightOn;
    keyL_prev = lNow;

    bool mNow = (glfwGetKey(win, GLFW_KEY_M) == GLFW_PRESS);
    if (mNow && !keyM_prev) materialOn = !materialOn;
    keyM_prev = mNow;

    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);
}

// ----------------------------------------------------------------
//  main
// ----------------------------------------------------------------

int main()
{
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 
    GLFWwindow* win = glfwCreateWindow(1024, 768,
        "Haruna Muhammad Idris - SF704D - Assignment 3 |  drag=orbit  scroll=zoom  L=light  M=magenta ", nullptr, nullptr);

    if (!win) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);


    

    // Register mouse callbacks
    glfwSetMouseButtonCallback(win, mouseButtonCallback);
    glfwSetCursorPosCallback(win, cursorPosCallback);
    glfwSetScrollCallback(win, scrollCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW init failed\n"; return -1; }

    glEnable(GL_DEPTH_TEST);

    // Shaders
    GLuint cubeProg = buildProgram("shaders/cube.vert", "shaders/cube.frag");
    GLuint sunProg = buildProgram("shaders/sun.vert", "shaders/sun.frag");

    // Cube VAO
    GLuint cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO); glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Sun sphere VAO
    auto sphereVerts = buildSphere(20, 20, 0.25f);
    GLuint sunVAO, sunVBO;
    glGenVertexArrays(1, &sunVAO); glGenBuffers(1, &sunVBO);
    glBindVertexArray(sunVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sunVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVerts.size() * sizeof(float), sphereVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    GLuint sunTex = loadTexture("textures/sun.jpg");

    // Cube positions: side=1, gap=1, middle cube at origin
    glm::vec3 cubePositions[3] = {
        glm::vec3(-2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
    };

    float lastTime = (float)glfwGetTime();

    // Initialise last mouse position so first drag is smooth
    glfwGetCursorPos(win, &lastMouseX, &lastMouseY);

    // ---------- render loop ----------
    while (!glfwWindowShouldClose(win))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        glfwPollEvents();
        processInput(win);

        // Animate light
        lightAngle += 0.5f * dt;

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int fbW, fbH;
        glfwGetFramebufferSize(win, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        // Camera position on cylinder surface, always looking at origin
        glm::vec3 camPos(
            camRadius * cosf(camAngle),
            camHeight,
            camRadius * sinf(camAngle)
        );
        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f),
            float(fbW) / float(fbH), 0.1f, 100.0f);

        // Light position (circle in XZ plane at height 2)
        glm::vec3 lightPos(
            lightRadius * cosf(lightAngle),
            2.0f,
            lightRadius * sinf(lightAngle)
        );

        // ---- Draw cubes ----
        glUseProgram(cubeProg);
        glUniformMatrix4fv(glGetUniformLocation(cubeProg, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(cubeProg, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1i(glGetUniformLocation(cubeProg, "lightOn"), lightOn ? 1 : 0);
        glUniform1i(glGetUniformLocation(cubeProg, "materialOn"), materialOn ? 1 : 0);
        glUniform3fv(glGetUniformLocation(cubeProg, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3f(glGetUniformLocation(cubeProg, "lightColor"), 1.0f, 1.0f, 0.0f);  // yellow
        glUniform3f(glGetUniformLocation(cubeProg, "objectColor"), 1.0f, 0.0f, 1.0f); // magenta

        glBindVertexArray(cubeVAO);
        for (int i = 0; i < 3; ++i) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
            glUniformMatrix4fv(glGetUniformLocation(cubeProg, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);

        // ---- Draw sun sphere (only when light is on) ----
        if (lightOn) {
            glUseProgram(sunProg);
            glUniformMatrix4fv(glGetUniformLocation(sunProg, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(sunProg, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
            glm::mat4 sunModel = glm::translate(glm::mat4(1.0f), lightPos);
            glUniformMatrix4fv(glGetUniformLocation(sunProg, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sunTex);
            glUniform1i(glGetUniformLocation(sunProg, "sunTexture"), 0);
            glBindVertexArray(sunVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(sphereVerts.size() / 5));
            glBindVertexArray(0);
        }

        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1, &cubeVAO); glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &sunVAO);  glDeleteBuffers(1, &sunVBO);
    glDeleteTextures(1, &sunTex);
    glDeleteProgram(cubeProg); glDeleteProgram(sunProg);
    glfwTerminate();
    return 0;
}