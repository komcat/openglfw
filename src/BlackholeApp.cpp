#include "BlackholeApp.h"
#include "LightRay.h"
#include <iostream>
#include <cmath>

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
  , blackholeRadius(0.288f)
  , blackholeMass(0.22f)
  , time(0.0f)
  , raySpeed(0.84f)
  , timeSinceLastSpawn(0.0f)
  , spawnInterval(0.2f)
  , currentPattern(RayFactory::SpawnPattern::RADIAL) {

  g_App = this;  // Set global pointer for callback
  rayFactory = std::make_unique<RayFactory>();
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

void BlackholeApp::SetSpawnPattern(RayFactory::SpawnPattern pattern) {
  currentPattern = pattern;
  rayFactory->SetSpawnStrategy(pattern);

  // Clear existing rays and spawn new batch with new pattern
  rays.clear();
  SpawnRayBatch();

  std::cout << "Spawn pattern changed to: " << rayFactory->GetCurrentStrategyName() << std::endl;
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
    "Black Hole Light Ray Simulation - Factory Pattern", NULL, NULL);
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

void BlackholeApp::InitRays() {
  rays.clear();
  timeSinceLastSpawn = 0.0f;

  // Start with an initial batch of rays using the factory
  SpawnRayBatch();

  std::cout << "Ray spawning initialized using " << rayFactory->GetCurrentStrategyName()
    << " strategy" << std::endl;
  std::cout << "Max rays: " << MAX_RAYS
    << ", Spawn interval: " << spawnInterval << " seconds" << std::endl;
}

void BlackholeApp::SpawnRayBatch() {
  // Don't spawn if we're at max capacity
  if (rays.size() >= MAX_RAYS) {
    return;
  }

  // Calculate how many rays we can spawn
  int raysToSpawn = std::min(RAYS_PER_SPAWN, MAX_RAYS - (int)rays.size());

  // Use factory to create new rays
  auto newRays = rayFactory->CreateRays(raysToSpawn, raySpeed, 10);

  // Move new rays into the main container
  for (auto& ray : newRays) {
    rays.push_back(std::move(ray));
  }
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

  // Draw event horizon edge ring (red boundary)
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

void BlackholeApp::DrawRays() {
  glUseProgram(shaderProgram);
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

  // Pre-allocate for performance
  std::vector<float> pointVertices;
  pointVertices.reserve(rays.size() * 2);

  for (const auto& ray : rays) {
    if (ray->IsAbsorbed()) {
      continue;
    }

    const auto& segments = ray->GetSegments();
    if (segments.empty()) continue;

    // Head position
    pointVertices.push_back(segments[0].x);
    pointVertices.push_back(segments[0].y);
  }

  if (pointVertices.empty()) return;

  glBufferSubData(GL_ARRAY_BUFFER, 0,
    pointVertices.size() * sizeof(float), pointVertices.data());

  // Bright cyan particles
  glUniform4f(glGetUniformLocation(shaderProgram, "u_Color"), 0.8f, 1.0f, 1.0f, 0.9f);

  glEnable(GL_POINT_SMOOTH);
  glPointSize(2.0f);  // Smaller points for many particles
  glDrawArrays(GL_POINTS, 0, pointVertices.size() / 2);
  glPointSize(1.0f);
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

  // Change spawn pattern with number keys
  static bool key1WasPressed = false;
  static bool key2WasPressed = false;
  static bool key3WasPressed = false;
  static bool key4WasPressed = false;

  bool key1Pressed = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
  bool key2Pressed = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
  bool key3Pressed = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS);
  bool key4Pressed = (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS);

  if (key1Pressed && !key1WasPressed) {
    SetSpawnPattern(RayFactory::SpawnPattern::LEFT_EDGE);
  }
  if (key2Pressed && !key2WasPressed) {
    SetSpawnPattern(RayFactory::SpawnPattern::FOUR_EDGES);
  }
  if (key3Pressed && !key3WasPressed) {
    SetSpawnPattern(RayFactory::SpawnPattern::RADIAL);
  }
  if (key4Pressed && !key4WasPressed) {
    SetSpawnPattern(RayFactory::SpawnPattern::SPIRAL);
  }

  key1WasPressed = key1Pressed;
  key2WasPressed = key2Pressed;
  key3WasPressed = key3Pressed;
  key4WasPressed = key4Pressed;

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

  // Adjust spawn interval with +/- keys
  if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS ||
    glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
    spawnInterval = std::max(0.05f, spawnInterval - 0.01f);
    std::cout << "Spawn interval decreased to: " << spawnInterval << " seconds" << std::endl;
  }
  if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS ||
    glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
    spawnInterval = std::min(2.0f, spawnInterval + 0.01f);
    std::cout << "Spawn interval increased to: " << spawnInterval << " seconds" << std::endl;
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
    std::cout << "\n=== Current Parameters ===" << std::endl;
    std::cout << "Spawn Pattern: " << rayFactory->GetCurrentStrategyName() << std::endl;
    std::cout << "Black hole mass: " << blackholeMass << std::endl;
    std::cout << "Black hole radius: " << blackholeRadius << std::endl;
    std::cout << "Light speed: " << raySpeed << std::endl;
    std::cout << "Gravity multiplier: " << LightRay::GetGravityMultiplier() << std::endl;
    std::cout << "Max force cap: " << LightRay::GetMaxForce() << std::endl;
    std::cout << "Force exponent: " << LightRay::GetForceExponent() << std::endl;
    std::cout << "Max rays: " << MAX_RAYS << std::endl;
    std::cout << "Spawn interval: " << spawnInterval << " seconds" << std::endl;
    std::cout << "=========================" << std::endl;
  }

  pKeyWasPressed = pKeyIsPressed;
}

void BlackholeApp::Update(float deltaTime) {
  time += deltaTime;
  timeSinceLastSpawn += deltaTime;

  // Spawn new rays periodically
  if (timeSinceLastSpawn >= spawnInterval) {
    SpawnRayBatch();
    timeSinceLastSpawn = 0.0f;
  }

  // Update all rays and remove those that need resetting
  for (auto it = rays.begin(); it != rays.end(); ) {
    auto& ray = *it;

    // Update the ray
    ray->Update(deltaTime, blackholePos, blackholeMass, blackholeRadius);

    // Remove rays that go off screen or have been absorbed too long
    if (ray->NeedsReset() && !ray->IsAbsorbed()) {
      it = rays.erase(it);
    }
    else if (ray->ShouldRespawn()) {
      it = rays.erase(it);
    }
    else {
      ++it;
    }
  }

  // Optional: Print current ray count periodically for debugging
  static float printTimer = 0.0f;
  printTimer += deltaTime;
  if (printTimer > 5.0f) {  // Print every 5 seconds
    std::cout << "Active rays: " << rays.size() << "/" << MAX_RAYS
      << " (" << rayFactory->GetCurrentStrategyName() << " pattern)" << std::endl;
    printTimer = 0.0f;
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