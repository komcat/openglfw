#include "LightFieldGrid.h"
#include <glad/glad.h>
#include <algorithm>
#include <cmath>

LightFieldGrid::LightFieldGrid()
  : decayRate(0.985f)      // Slow fade for trail effect
  , maxBrightness(5.0f)    // Cap brightness to prevent oversaturation
  , worldSize(4.0f)        // World spans from -2 to 2
  , VAO(0)
  , VBO(0)
  , EBO(0) {

  // Initialize grid with zeros
  grid.resize(GRID_SIZE);
  for (int i = 0; i < GRID_SIZE; i++) {
    grid[i].resize(GRID_SIZE, 0.0f);
  }
}

LightFieldGrid::~LightFieldGrid() {
  if (VAO) glDeleteVertexArrays(1, &VAO);
  if (VBO) glDeleteBuffers(1, &VBO);
  if (EBO) glDeleteBuffers(1, &EBO);
}

bool LightFieldGrid::Initialize() {
  // Create vertex data for grid cells
  // Each cell is a quad (4 vertices)
  vertices.clear();
  indices.clear();

  float cellSize = worldSize / GRID_SIZE;

  // Reserve space
  vertices.reserve(GRID_SIZE * GRID_SIZE * 4 * 5); // 4 verts * 5 floats per cell
  indices.reserve(GRID_SIZE * GRID_SIZE * 6);      // 2 triangles * 3 indices per cell

  // Generate vertices and indices for all cells
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      float worldX = -worldSize / 2.0f + x * cellSize;
      float worldY = -worldSize / 2.0f + y * cellSize;

      int baseIndex = (y * GRID_SIZE + x) * 4;

      // Add 4 vertices for this cell (positions + colors)
      // Bottom left
      vertices.push_back(worldX);
      vertices.push_back(worldY);
      vertices.push_back(0.0f); // R
      vertices.push_back(0.0f); // G
      vertices.push_back(0.0f); // B

      // Bottom right
      vertices.push_back(worldX + cellSize);
      vertices.push_back(worldY);
      vertices.push_back(0.0f);
      vertices.push_back(0.0f);
      vertices.push_back(0.0f);

      // Top right
      vertices.push_back(worldX + cellSize);
      vertices.push_back(worldY + cellSize);
      vertices.push_back(0.0f);
      vertices.push_back(0.0f);
      vertices.push_back(0.0f);

      // Top left
      vertices.push_back(worldX);
      vertices.push_back(worldY + cellSize);
      vertices.push_back(0.0f);
      vertices.push_back(0.0f);
      vertices.push_back(0.0f);

      // Add indices for 2 triangles
      indices.push_back(baseIndex);
      indices.push_back(baseIndex + 1);
      indices.push_back(baseIndex + 2);

      indices.push_back(baseIndex);
      indices.push_back(baseIndex + 2);
      indices.push_back(baseIndex + 3);
    }
  }

  // Create OpenGL objects
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
    vertices.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
    indices.data(), GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Color attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
    (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  return true;
}

void LightFieldGrid::Clear() {
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      grid[y][x] = 0.0f;
    }
  }
}

glm::ivec2 LightFieldGrid::WorldToGrid(glm::vec2 worldPos) const {
  // Convert world coordinates (-2 to 2) to grid coordinates (0 to GRID_SIZE-1)
  float normalizedX = (worldPos.x + worldSize / 2.0f) / worldSize;
  float normalizedY = (worldPos.y + worldSize / 2.0f) / worldSize;

  int gridX = (int)(normalizedX * GRID_SIZE);
  int gridY = (int)(normalizedY * GRID_SIZE);

  // Clamp to grid bounds
  gridX = std::max(0, std::min(GRID_SIZE - 1, gridX));
  gridY = std::max(0, std::min(GRID_SIZE - 1, gridY));

  return glm::ivec2(gridX, gridY);
}

void LightFieldGrid::AccumulateLineBresenham(int x0, int y0, int x1, int y1, float intensity) {
  // Bresenham's line algorithm to accumulate intensity along a line
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    // Check bounds and accumulate
    if (x0 >= 0 && x0 < GRID_SIZE && y0 >= 0 && y0 < GRID_SIZE) {
      grid[y0][x0] += intensity;
      grid[y0][x0] = std::min(grid[y0][x0], maxBrightness);
    }

    if (x0 == x1 && y0 == y1) break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void LightFieldGrid::AccumulateRaySegment(glm::vec2 start, glm::vec2 end, float intensity) {
  // Convert world coordinates to grid coordinates
  glm::ivec2 gridStart = WorldToGrid(start);
  glm::ivec2 gridEnd = WorldToGrid(end);

  // Use Bresenham's algorithm to accumulate along the line
  AccumulateLineBresenham(gridStart.x, gridStart.y, gridEnd.x, gridEnd.y, intensity);
}

void LightFieldGrid::Update(float deltaTime) {
  // Apply decay to all cells (creates trail effect)
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      grid[y][x] *= decayRate;

      // Clean up very small values
      if (grid[y][x] < 0.001f) {
        grid[y][x] = 0.0f;
      }
    }
  }

  // Update vertex colors based on grid intensity
  UpdateVertices();
}

glm::vec3 LightFieldGrid::IntensityToColor(float intensity) const {
  // Map intensity to color gradient
  // 0 -> black
  // low -> dark blue
  // medium -> cyan
  // high -> white

  float normalized = intensity / maxBrightness;
  normalized = std::min(1.0f, normalized);

  glm::vec3 color;

  if (normalized < 0.25f) {
    // Black to dark blue
    float t = normalized * 4.0f;
    color = glm::vec3(0.0f, 0.0f, t * 0.3f);
  }
  else if (normalized < 0.5f) {
    // Dark blue to blue
    float t = (normalized - 0.25f) * 4.0f;
    color = glm::vec3(0.0f, t * 0.2f, 0.3f + t * 0.4f);
  }
  else if (normalized < 0.75f) {
    // Blue to cyan
    float t = (normalized - 0.5f) * 4.0f;
    color = glm::vec3(t * 0.3f, 0.2f + t * 0.5f, 0.7f + t * 0.3f);
  }
  else {
    // Cyan to white
    float t = (normalized - 0.75f) * 4.0f;
    color = glm::vec3(0.3f + t * 0.7f, 0.7f + t * 0.3f, 1.0f);
  }

  return color;
}

void LightFieldGrid::UpdateVertices() {
  // Update color values in vertex buffer based on grid intensities
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      float intensity = grid[y][x];
      glm::vec3 color = IntensityToColor(intensity);

      // Calculate base index for this cell's vertices
      int cellIndex = y * GRID_SIZE + x;
      int baseVertexIndex = cellIndex * 4 * 5; // 4 vertices * 5 floats each

      // Update colors for all 4 vertices of this cell
      for (int v = 0; v < 4; v++) {
        int colorIndex = baseVertexIndex + v * 5 + 2; // +2 to skip x,y
        vertices[colorIndex] = color.r;
        vertices[colorIndex + 1] = color.g;
        vertices[colorIndex + 2] = color.b;
      }
    }
  }

  // Update VBO with new colors
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void LightFieldGrid::Render(unsigned int shaderProgram) {
  glUseProgram(shaderProgram);

  // Set uniform for grid rendering mode
  glUniform4f(glGetUniformLocation(shaderProgram, "u_Color"), 1.0f, 1.0f, 1.0f, 1.0f);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}