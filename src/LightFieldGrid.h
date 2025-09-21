#pragma once

#include <glm/glm.hpp>
#include <vector>

class LightFieldGrid {
public:
  static const int GRID_SIZE = 100;  // 100x100 grid

  LightFieldGrid();
  ~LightFieldGrid();

  // Initialize OpenGL resources for rendering
  bool Initialize();

  // Clear the grid
  void Clear();

  // Add ray contribution to grid cells along a line segment
  void AccumulateRaySegment(glm::vec2 start, glm::vec2 end, float intensity = 1.0f);

  // Update the grid (apply decay, etc.)
  void Update(float deltaTime);

  // Render the grid as colored quads
  void Render(unsigned int shaderProgram);

  // Convert world coordinates to grid coordinates
  glm::ivec2 WorldToGrid(glm::vec2 worldPos) const;

  // Get/Set decay rate
  void SetDecayRate(float rate) { decayRate = rate; }
  float GetDecayRate() const { return decayRate; }

  // Get/Set max brightness
  void SetMaxBrightness(float max) { maxBrightness = max; }
  float GetMaxBrightness() const { return maxBrightness; }

private:
  // Grid data - stores accumulated light intensity
  std::vector<std::vector<float>> grid;

  // Rendering
  unsigned int VAO, VBO, EBO;
  std::vector<float> vertices;
  std::vector<unsigned int> indices;

  // Parameters
  float decayRate;        // How fast cells fade (0.98 = slow fade)
  float maxBrightness;    // Maximum brightness cap
  float worldSize;        // Size of world space (-2 to 2)

  // Helper methods
  void UpdateVertices();
  glm::vec3 IntensityToColor(float intensity) const;
  void AccumulateLineBresenham(int x0, int y0, int x1, int y1, float intensity);
};