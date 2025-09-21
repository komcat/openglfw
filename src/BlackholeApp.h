#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <memory>
#include <string>
#include "LightRay.h"
#include "RayFactory.h"

class BlackholeApp {
public:
  BlackholeApp(int width, int height);
  ~BlackholeApp();

  // Initialize the application
  bool Initialize();

  // Main render loop
  void Render();

  // Update physics/animation
  void Update(float deltaTime);

  // Handle input
  void ProcessInput(GLFWwindow* window);

  // Check if app should close
  bool ShouldClose() const;

  // Get window handle
  GLFWwindow* GetWindow() const { return window; }

  // Window resize callback
  static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

  // Change spawn pattern at runtime
  void SetSpawnPattern(RayFactory::SpawnPattern pattern);

private:
  // Window dimensions
  int windowWidth;
  int windowHeight;

  // OpenGL handles
  GLFWwindow* window;
  unsigned int shaderProgram;
  unsigned int lineVAO, lineVBO;

  // Black hole parameters - ALWAYS CENTERED
  glm::vec2 blackholePos;      // Always (0, 0) in normalized coords
  float blackholeRadius;        // Visual radius of black hole (event horizon)
  float blackholeMass;          // Mass (affects gravity strength)

  // Light rays
  std::vector<std::unique_ptr<LightRay>> rays;

  // Ray factory for creating rays
  std::unique_ptr<RayFactory> rayFactory;

  // Animation
  float time;
  float raySpeed;               // Speed of light (adjustable)

  // Shader sources
  static const char* vertexShaderSource;
  static const char* fragmentShaderSource;

  // Ray spawning parameters
  static const int MAX_RAYS = 10000;      // Maximum rays on screen
  static const int RAYS_PER_SPAWN = 500;  // How many rays to spawn each time
  float timeSinceLastSpawn;               // Time accumulator for spawning
  float spawnInterval;                    // Time between spawns

  // Current spawn pattern
  RayFactory::SpawnPattern currentPattern;

  // Helper methods
  bool InitWindow();
  bool InitShaders();
  bool InitGeometry();
  void InitRays();
  void UpdateProjectionMatrix();
  void UpdateRaySpeed(float newSpeed);
  void DrawBlackhole();
  void DrawRays();
  void SpawnRayBatch();

  // Shader compilation helpers
  unsigned int CompileShader(unsigned int type, const char* source);
  unsigned int CreateShaderProgram(const char* vertSource, const char* fragSource);
};