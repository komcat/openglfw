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

private:
  // Window dimensions
  int windowWidth;
  int windowHeight;

  // OpenGL handles
  GLFWwindow* window;
  unsigned int shaderProgram;
  unsigned int lineVAO, lineVBO;

  // Black hole parameters
  glm::vec2 blackholePos;      // Position in screen space [-1, 1]
  float blackholeRadius;        // Visual radius of black hole (event horizon)
  float blackholeMass;          // Mass (affects gravity strength)

  // Light rays
  static const int NUM_RAYS = 80;
  std::vector<std::unique_ptr<LightRay>> rays;

  // Animation
  float time;
  float raySpeed;               // Speed of light (adjustable)

  // Shader sources
  static const char* vertexShaderSource;
  static const char* fragmentShaderSource;

  // Helper methods
  bool InitWindow();
  bool InitShaders();
  bool InitGeometry();
  void InitRays();
  void UpdateRaySpeed(float newSpeed);
  void DrawBlackhole();
  void DrawRays();
  unsigned int CompileShader(unsigned int type, const char* source);
  unsigned int CreateShaderProgram(const char* vertSource, const char* fragSource);
};