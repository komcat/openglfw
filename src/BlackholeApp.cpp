#include "BlackholeApp.h"
#include "LightRay.h"
#include <iostream>
#include <cmath>

// Define PI if not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

uniform vec4 u_Color;  // Changed from vec3 to vec4 to include alpha

void main() {
    FragColor = u_Color;  // Now includes alpha channel
}
)";

BlackholeApp::BlackholeApp(int width, int height)
  : windowWidth(width)
  , windowHeight(height)
  , window(nullptr)
  , shaderProgram(0)
  , lineVAO(0)
  , lineVBO(0)
  , blackholePos(0.5f, 0.0f)  // Right side of screen
  , blackholeRadius(0.15f)     // 15% of screen height
  , blackholeMass(1.0f)        // Increase this for stronger gravity (try 0.5 to 2.0)
  , time(0.0f)
  , raySpeed(0.3f) {
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

// In BlackholeApp.cpp, replace the Initialize() method to fix aspect ratio:

bool BlackholeApp::Initialize() {
  if (!InitWindow()) {
    std::cerr << "Failed to initialize window" << std::endl;
    return false;
  }

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

  // Set up projection matrix with proper aspect ratio
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

  // Enable blending for transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Enable line smoothing
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  return true;
}

// Also update DrawBlackhole() to use vec4:

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

  // Draw photon sphere - golden ring with slight transparency
  glUniform4f(glGetUniformLocation(shaderProgram, "u_Color"), 1.0f, 0.8f, 0.3f, 0.8f);
  glLineWidth(1.5f);

  std::vector<float> photonRingVertices;
  float photonRadius = blackholeRadius * 1.5f;
  for (int i = 0; i <= segments; i++) {
    float angle = 2.0f * M_PI * i / segments;
    float x = blackholePos.x + photonRadius * cosf(angle);
    float y = blackholePos.y + photonRadius * sinf(angle);
    photonRingVertices.push_back(x);
    photonRingVertices.push_back(y);
  }

  glBufferSubData(GL_ARRAY_BUFFER, 0,
    photonRingVertices.size() * sizeof(float), photonRingVertices.data());
  glDrawArrays(GL_LINE_LOOP, 0, segments + 1);

  // Draw event horizon edge ring
  std::vector<float> ringVertices;
  for (int i = 0; i <= segments; i++) {
    float angle = 2.0f * M_PI * i / segments;
    float x = blackholePos.x + blackholeRadius * cosf(angle);
    float y = blackholePos.y + blackholeRadius * sinf(angle);
    ringVertices.push_back(x);
    ringVertices.push_back(y);
  }

  glUniform4f(glGetUniformLocation(shaderProgram, "u_Color"), 0.8f, 0.2f, 0.2f, 0.9f);
  glLineWidth(2.0f);
  glBufferSubData(GL_ARRAY_BUFFER, 0,
    ringVertices.size() * sizeof(float), ringVertices.data());
  glDrawArrays(GL_LINE_LOOP, 0, segments + 1);
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
    "Black Hole Light Ray Simulation", NULL, NULL);
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

  // Pre-allocate larger buffer for long ray vertices
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 10000 * 2, nullptr, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return true;
}



void BlackholeApp::InitRays() {
  rays.clear();

  float spacing = 2.0f / (NUM_RAYS + 1);  // Evenly space rays vertically

  for (int i = 0; i < NUM_RAYS; i++) {
    float y = -1.0f + spacing * (i + 1);  // Evenly spaced vertically

    // No angle variation - all rays travel perfectly horizontal
    float angle = 0.0f;  // 0 radians = straight to the right

    // Create rays with 500 segments for very long appearance
    rays.push_back(std::make_unique<LightRay>(y, raySpeed, 500, angle));
  }
}

void BlackholeApp::UpdateRaySpeed(float newSpeed) {
  // Update speed for all existing rays
  for (auto& ray : rays) {
    ray->SetSpeed(newSpeed);
  }
}


void BlackholeApp::DrawRays() {
  glUseProgram(shaderProgram);
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

  for (const auto& ray : rays) {
    const auto& segments = ray->GetSegments();
    if (segments.size() < 2) continue;

    std::vector<float> lineVertices;
    for (const auto& segment : segments) {
      lineVertices.push_back(segment.x);
      lineVertices.push_back(segment.y);
    }

    // Set ray color with transparency (4th value is alpha: 0=invisible, 1=opaque)
    if (ray->IsAbsorbed()) {
      // Redshifted light at event horizon - semi-transparent red
      glUniform4f(glGetUniformLocation(shaderProgram, "u_Color"), 0.5f, 0.1f, 0.1f, 0.6f);
    }
    else {
      // Light gray with transparency for normal rays
      glUniform4f(glGetUniformLocation(shaderProgram, "u_Color"), 0.8f, 0.8f, 0.8f, 0.5f);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0,
      lineVertices.size() * sizeof(float), lineVertices.data());

    glLineWidth(2.0f);
    glDrawArrays(GL_LINE_STRIP, 0, lineVertices.size() / 2);

    // HEAD DRAWING REMOVED - No more white dots at ray heads
    // The rays now appear as continuous beams without emphasized endpoints
  }
}


void BlackholeApp::ProcessInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  // Move black hole with arrow keys
  float moveSpeed = 0.01f;
  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    blackholePos.x -= moveSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    blackholePos.x += moveSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    blackholePos.y += moveSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    blackholePos.y -= moveSpeed;
  }


  // Adjust mass with Q/E keys (existing)
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    blackholeMass = std::max(0.1f, blackholeMass - 0.01f);
    std::cout << "Black hole mass decreased to: " << blackholeMass << std::endl;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    blackholeMass = std::min(5.0f, blackholeMass + 0.01f);  // Increased max to 5
    std::cout << "Black hole mass increased to: " << blackholeMass << std::endl;
  }

  // NEW: Gravity multiplier with D/F keys (accelerate/decelerate gravity effect)
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

  // NEW: Max force cap with C/V keys
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

  // NEW: Force exponent with G/H keys (changes falloff curve)
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

  // Adjust black hole radius (size) with Z/X keys
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

  // Reset with R key or SPACE bar (keep current mass)
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS ||
    glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    blackholePos = glm::vec2(0.5f, 0.0f);
    // Don't reset blackholeMass, blackholeRadius, or raySpeed - keep current values
    InitRays();
    std::cout << "Simulation reset (keeping: mass=" << blackholeMass
      << ", radius=" << blackholeRadius
      << ", light_speed=" << raySpeed << ")" << std::endl;
  }


  // Update the P key output to include new parameters
  static bool pKeyPressed = false;
  if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pKeyPressed) {
    pKeyPressed = true;
    std::cout << "\n=== Current Parameters ===" << std::endl;
    std::cout << "Black hole mass: " << blackholeMass << std::endl;
    std::cout << "Black hole radius: " << blackholeRadius << std::endl;
    std::cout << "Light speed: " << raySpeed << std::endl;
    std::cout << "Gravity multiplier: " << LightRay::GetGravityMultiplier() << std::endl;
    std::cout << "Max force cap: " << LightRay::GetMaxForce() << std::endl;
    std::cout << "Force exponent: " << LightRay::GetForceExponent() << std::endl;
    std::cout << "Photon sphere radius: " << blackholeRadius * 1.5f << std::endl;
    std::cout << "=========================" << std::endl;
  }
  else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
    pKeyPressed = false;
  }
}

void BlackholeApp::Update(float deltaTime) {
  time += deltaTime;

  // Update all rays
  for (int i = 0; i < rays.size(); i++) {
    auto& ray = rays[i];

    // Store if ray was absorbed before update
    bool wasAbsorbed = ray->IsAbsorbed();

    // Update the ray
    ray->Update(deltaTime, blackholePos, blackholeMass, blackholeRadius);

    // If ray just got absorbed or went off screen, create a new one at the same Y position
    if ((ray->IsAbsorbed() && !wasAbsorbed) || ray->NeedsReset()) {
      // Calculate the Y position for this ray index
      float spacing = 2.0f / (NUM_RAYS + 1);
      float y = -1.0f + spacing * (i + 1);

      // Create a new ray at the same vertical position
      rays[i] = std::make_unique<LightRay>(y, raySpeed, 500, 0.0f);  // 0 angle for horizontal
    }
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
