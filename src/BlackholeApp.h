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
#include "LightFieldGrid.h"

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

private:
  // Window dimensions
  int windowWidth;
  int windowHeight;

  // OpenGL handles
  GLFWwindow* window;
  unsigned int shaderProgram;
  unsigned int gridShaderProgram;  // New shader for grid rendering
  unsigned int lineVAO, lineVBO;

  // Black hole parameters - ALWAYS CENTERED
  glm::vec2 blackholePos;      // Always (0, 0) in normalized coords
  float blackholeRadius;        // Visual radius of black hole (event horizon)
  float blackholeMass;          // Mass (affects gravity strength)

  // Light rays
  static const int NUM_RAYS = 8000;  // 2000 rays for dense field
  std::vector<std::unique_ptr<LightRay>> rays;

  // Light field grid for density visualization
  std::unique_ptr<LightFieldGrid> lightField;

  // Animation
  float time;
  float raySpeed;               // Speed of light (adjustable)
  float zoomLevel;              // Zoom level for camera

  // Shader sources
  static const char* vertexShaderSource;
  static const char* fragmentShaderSource;
  static const char* gridVertexShaderSource;
  static const char* gridFragmentShaderSource;

  // Helper methods
  bool InitWindow();
  bool InitShaders();
  bool InitGeometry();
  void InitRays();
  void UpdateProjectionMatrix();
  void UpdateRaySpeed(float newSpeed);
  void DrawBlackhole();
  void DrawRays();
  void UpdateLightField();
  unsigned int CompileShader(unsigned int type, const char* source);
  unsigned int CreateShaderProgram(const char* vertSource, const char* fragSource);
};