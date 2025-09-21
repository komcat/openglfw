#include "BlackholeApp.h"
#include "LightRay.h"
#include <iostream>
#include <cmath>
#include <random>

// Define PI if not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Static member for callback
static BlackholeApp* g_App = nullptr;

// Vertex shader - simple 2D positions
const char* BlackholeApp::vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 u_Projection;

void main() {
    gl_Position = u_Projection * vec4(aPos, 0.0, 1.0);
}
)";

// Fragment shader - simple color output
const char* BlackholeApp::fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 u_Color;

void main() {
    FragColor = u_Color;
}
)";

BlackholeApp::BlackholeApp(int width, int height)
  : windowWidth(width)
  , windowHeight(height)
  , window(nullptr)
  , shaderProgram(0)
  , lineVAO(0)
  , lineVBO(0)
  , blackholePos(0.0f, 0.0f)  // ALWAYS centered at origin
  , blackholeRadius(0.288f)    // Your preferred radius
  , blackholeMass(0.22f)       // Your preferred mass
  , time(0.0f)
  , raySpeed(0.84f) {          // Your preferred speed (0.839999 rounded)
  g_App = this;  // Set global pointer for callback
}

BlackholeApp::~BlackholeApp() {
  if (lineVAO) glDeleteVertexArrays(1, &lineVAO);
  if (lineVBO) glDeleteBuffers(1, &lineVBO);
  if (shaderProgram) glDeleteProgram(shaderProgram);
  if (window) {
    glfwDestroyWindow(window);
    glfwTerminate();
  }
}

// Window resize callback implementation
void BlackholeApp::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
  if (g_App) {
    g_App->windowWidth = width;
    g_App->windowHeight = height;
    glViewport(0, 0, width, height);
    g_App->UpdateProjectionMatrix();
  }
}

// Update projection matrix
void BlackholeApp::UpdateProjectionMatrix() {
  glUseProgram(shaderProgram);
  float aspectRatio = (float)windowWidth / (float)windowHeight;
  glm::mat4 projection;

  if (aspectRatio > 1.0f) {
    projection = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f);
  }
  else {
    projection = glm::ortho(-1.0f, 1.0f, -1.0f / aspectRatio, 1.0f / aspectRatio);
  }

  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Projection"),
    1, GL_FALSE, glm::value_ptr(projection));
}

bool BlackholeApp::Initialize() {
  if (!InitWindow()) {
    std::cerr << "Failed to initialize window" << std::endl;
    return false;
  }

  // Set the framebuffer size callback
  glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

  if (!InitShaders()) {
    std::cerr << "Failed to initialize shaders" << std::endl;
    return false;
  }

  if (!InitGeometry()) {
    std::cerr << "Failed to initialize geometry" << std::endl;
    return false;
  }

  // Initialize light rays
  InitRays();

  // Set up initial projection matrix
  UpdateProjectionMatrix();

  // Enable blending for transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Enable line smoothing
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  return true;
}

bool BlackholeApp::InitWindow() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(windowWidth, windowHeight,
    "Black Hole Radial Light Ray Simulation", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return false;
  }

  glViewport(0, 0, windowWidth, windowHeight);
  glfwSwapInterval(1);

  return true;
}

bool BlackholeApp::InitShaders() {
  shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
  return shaderProgram != 0;
}

bool BlackholeApp::InitGeometry() {
  // Create VAO and VBO for line drawing
  glGenVertexArrays(1, &lineVAO);
  glGenBuffers(1, &lineVBO);

  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

  // Pre-allocate larger buffer for many rays
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 20000 * 2, nullptr, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return true;
}

// InitRays() for parallel beams from 4 directions
void BlackholeApp::InitRays() {
  rays.clear();

  // Random number generation for variations
  std::random_device rd;
  std::mt19937 gen(rd());

  // Distributions for position and angle noise
  std::uniform_real_distribution<float> posNoise(-0.02f, 0.02f);     // Small position variation
  std::uniform_real_distribution<float> angleNoise(-0.01f, 0.01f);   // Very small angle variation
  std::uniform_real_distribution<float> speedNoise(0.9f, 1.1f);      // Speed variation

  int raysPerDirection = NUM_RAYS / 4;  // Divide rays among 4 directions

  // 1. LEFT TO RIGHT rays
  for (int i = 0; i < raysPerDirection; i++) {
    float spacing = 4.0f / raysPerDirection;
    float y = -2.0f + spacing * i + posNoise(gen);
    float x = -2.0f;  // Start from left edge

    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, y),              // Starting position
      raySpeed * speedNoise(gen),   // Speed
      500,                          // Segment count
      0.0f + angleNoise(gen)        // Angle: 0 = straight right
    ));
  }

  // 2. RIGHT TO LEFT rays
  for (int i = 0; i < raysPerDirection; i++) {
    float spacing = 4.0f / raysPerDirection;
    float y = -2.0f + spacing * i + posNoise(gen);
    float x = 2.0f;  // Start from right edge

    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, y),              // Starting position
      raySpeed * speedNoise(gen),   // Speed
      500,                          // Segment count
      M_PI + angleNoise(gen)        // Angle: π = straight left
    ));
  }

  // 3. TOP TO BOTTOM rays
  for (int i = 0; i < raysPerDirection; i++) {
    float spacing = 4.0f / raysPerDirection;
    float x = -2.0f + spacing * i + posNoise(gen);
    float y = 2.0f;  // Start from top edge

    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, y),                    // Starting position
      raySpeed * speedNoise(gen),         // Speed
      500,                                // Segment count
      -M_PI / 2.0f + angleNoise(gen)       // Angle: -π/2 = straight down
    ));
  }

  // 4. BOTTOM TO TOP rays
  for (int i = 0; i < raysPerDirection; i++) {
    float spacing = 4.0f / raysPerDirection;
    float x = -2.0f + spacing * i + posNoise(gen);
    float y = -2.0f;  // Start from bottom edge

    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, y),                   // Starting position
      raySpeed * speedNoise(gen),        // Speed
      500,                               // Segment count
      M_PI / 2.0f + angleNoise(gen)        // Angle: π/2 = straight up
    ));
  }

  std::cout << "Initialized " << NUM_RAYS << " rays in 4-directional grid pattern" << std::endl;
}

void BlackholeApp::DrawBlackhole() {
  const int segments = 128;
  std::vector<float> circleVertices;

  circleVertices.push_back(blackholePos.x);
  circleVertices.push_back(blackholePos.y);

  for (int i = 0; i <= segments; i++) {
    float angle = 2.0f * M_PI * i / segments;
    float x = blackholePos.x + blackholeRadius * cosf(angle);
    float y = blackholePos.y + blackholeRadius * sinf(angle);
    circleVertices.push_back(x);
    circleVertices.push_back(y);
  }

  glUseProgram(shaderProgram);
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

  glBufferSubData(GL_ARRAY_BUFFER, 0,
    circleVertices.size() * sizeof(float), circleVertices.data());

  // Draw filled black circle (fully opaque)
  glUniform4f(glGetUniformLocation(shaderProgram, "u_Color"), 0.0f, 0.0f, 0.0f, 1.0f);
  glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);

  // REMOVED: Event horizon red ring
}

void BlackholeApp::DrawRays() {
  glUseProgram(shaderProgram);
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

  for (const auto& ray : rays) {
    // Skip drawing absorbed rays - they disappear
    if (ray->IsAbsorbed()) {
      continue;
    }

    const auto& segments = ray->GetSegments();
    if (segments.size() < 2) continue;

    std::vector<float> lineVertices;
    for (const auto& segment : segments) {
      lineVertices.push_back(segment.x);
      lineVertices.push_back(segment.y);
    }

    // Light gray with 2% opacity (0.02 alpha) for all visible rays
    glUniform4f(glGetUniformLocation(shaderProgram, "u_Color"), 0.8f, 0.8f, 0.8f, 0.1f);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
      lineVertices.size() * sizeof(float), lineVertices.data());

    glLineWidth(1.0f);  // Thinner lines for 2000 rays
    glDrawArrays(GL_LINE_STRIP, 0, lineVertices.size() / 2);
  }
}

void BlackholeApp::UpdateRaySpeed(float newSpeed) {
  raySpeed = newSpeed;
  // Update speed for all existing rays
  for (auto& ray : rays) {
    ray->SetSpeed(newSpeed);
  }
}

void BlackholeApp::ProcessInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  // NO ARROW KEY MOVEMENT - Black hole always centered

  // Adjust mass with Q/E keys
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    blackholeMass = std::max(0.1f, blackholeMass - 0.01f);
    std::cout << "Black hole mass decreased to: " << blackholeMass << std::endl;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    blackholeMass = std::min(5.0f, blackholeMass + 0.01f);
    std::cout << "Black hole mass increased to: " << blackholeMass << std::endl;
  }

  // Gravity multiplier with D/F keys
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    float currentMult = LightRay::GetGravityMultiplier();
    LightRay::SetGravityMultiplier(std::max(0.1f, currentMult - 0.02f));
    std::cout << "Gravity multiplier decreased to: " << LightRay::GetGravityMultiplier() << std::endl;
  }
  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
    float currentMult = LightRay::GetGravityMultiplier();
    LightRay::SetGravityMultiplier(std::min(3.0f, currentMult + 0.02f));
    std::cout << "Gravity multiplier increased to: " << LightRay::GetGravityMultiplier() << std::endl;
  }

  // Max force cap with C/V keys
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
    float currentMax = LightRay::GetMaxForce();
    LightRay::SetMaxForce(std::max(1.0f, currentMax - 0.5f));
    std::cout << "Max force cap decreased to: " << LightRay::GetMaxForce() << std::endl;
  }
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
    float currentMax = LightRay::GetMaxForce();
    LightRay::SetMaxForce(std::min(50.0f, currentMax + 0.5f));
    std::cout << "Max force cap increased to: " << LightRay::GetMaxForce() << std::endl;
  }

  // Force exponent with G/H keys
  if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
    float currentExp = LightRay::GetForceExponent();
    LightRay::SetForceExponent(std::max(0.5f, currentExp - 0.05f));
    std::cout << "Force exponent decreased to: " << LightRay::GetForceExponent()
      << " (lower = stronger at distance)" << std::endl;
  }
  if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
    float currentExp = LightRay::GetForceExponent();
    LightRay::SetForceExponent(std::min(4.0f, currentExp + 0.05f));
    std::cout << "Force exponent increased to: " << LightRay::GetForceExponent()
      << " (higher = weaker at distance)" << std::endl;
  }

  // Adjust black hole radius with Z/X keys
  if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
    blackholeRadius = std::max(0.05f, blackholeRadius - 0.002f);
    std::cout << "Black hole radius decreased to: " << blackholeRadius << std::endl;
  }
  if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
    blackholeRadius = std::min(0.3f, blackholeRadius + 0.002f);
    std::cout << "Black hole radius increased to: " << blackholeRadius << std::endl;
  }

  // Adjust light speed with A/S keys
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    raySpeed = std::max(0.05f, raySpeed - 0.005f);
    UpdateRaySpeed(raySpeed);
    std::cout << "Light speed decreased to: " << raySpeed << std::endl;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    raySpeed = std::min(1.0f, raySpeed + 0.005f);
    UpdateRaySpeed(raySpeed);
    std::cout << "Light speed increased to: " << raySpeed << std::endl;
  }

  // Reset with R key or SPACE bar
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS ||
    glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    InitRays();
    std::cout << "Simulation reset (keeping current parameters)" << std::endl;
  }

  // Print parameters with P key (with debounce)
  static bool pKeyWasPressed = false;
  bool pKeyIsPressed = (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);

  if (pKeyIsPressed && !pKeyWasPressed) {
    // Key just pressed - print parameters
    std::cout << "\n=== Current Parameters ===" << std::endl;
    std::cout << "Black hole mass: " << blackholeMass << std::endl;
    std::cout << "Black hole radius: " << blackholeRadius << std::endl;
    std::cout << "Light speed: " << raySpeed << std::endl;
    std::cout << "Gravity multiplier: " << LightRay::GetGravityMultiplier() << std::endl;
    std::cout << "Max force cap: " << LightRay::GetMaxForce() << std::endl;
    std::cout << "Force exponent: " << LightRay::GetForceExponent() << std::endl;
    std::cout << "Number of rays: " << NUM_RAYS << std::endl;
    std::cout << "Respawn time: " << "0.1 seconds" << std::endl;
    std::cout << "=========================" << std::endl;
  }

  pKeyWasPressed = pKeyIsPressed;
}

void BlackholeApp::Update(float deltaTime) {
  time += deltaTime;

  // Update all rays
  for (int i = 0; i < rays.size(); i++) {
    auto& ray = rays[i];

    // Update the ray
    ray->Update(deltaTime, blackholePos, blackholeMass, blackholeRadius);
  }
}

void BlackholeApp::Render() {
  glClearColor(0.05f, 0.05f, 0.1f, 1.0f);  // Dark blue background
  glClear(GL_COLOR_BUFFER_BIT);

  // Draw rays first (so they appear behind the black hole)
  DrawRays();

  // Draw black hole on top
  DrawBlackhole();

  glfwSwapBuffers(window);
  glfwPollEvents();
}

bool BlackholeApp::ShouldClose() const {
  return glfwWindowShouldClose(window);
}

unsigned int BlackholeApp::CompileShader(unsigned int type, const char* source) {
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
    return 0;
  }

  return shader;
}

unsigned int BlackholeApp::CreateShaderProgram(const char* vertSource, const char* fragSource) {
  unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertSource);
  if (!vertexShader) return 0;

  unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragSource);
  if (!fragmentShader) {
    glDeleteShader(vertexShader);
    return 0;
  }

  unsigned int program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  int success;
  char infoLog[512];
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, 512, NULL, infoLog);
    std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
    glDeleteProgram(program);
    program = 0;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}