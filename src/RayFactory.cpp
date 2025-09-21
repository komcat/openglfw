#include "RayFactory.h"
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// LeftEdgeSpawnStrategy Implementation
std::vector<std::unique_ptr<LightRay>> LeftEdgeSpawnStrategy::CreateRayBatch(
  int count,
  float raySpeed,
  int segmentCount) {

  std::vector<std::unique_ptr<LightRay>> rays;
  rays.reserve(count);

  // Random number generation for variations
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> posNoise(-0.02f, 0.02f);
  std::uniform_real_distribution<float> angleNoise(-0.01f, 0.01f);
  std::uniform_real_distribution<float> speedNoise(0.95f, 1.05f);

  // Spawn rays distributed along the left edge
  for (int i = 0; i < count; i++) {
    float spacing = 4.0f / count;
    float y = -2.0f + spacing * i + spacing * 0.5f + posNoise(gen);
    float x = -2.0f;  // Left edge

    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, y),
      raySpeed * speedNoise(gen),
      segmentCount,
      0.0f + angleNoise(gen)  // Straight right
    ));
  }

  return rays;
}

// FourEdgeSpawnStrategy Implementation
std::vector<std::unique_ptr<LightRay>> FourEdgeSpawnStrategy::CreateRayBatch(
  int count,
  float raySpeed,
  int segmentCount) {

  std::vector<std::unique_ptr<LightRay>> rays;
  rays.reserve(count);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> posNoise(-0.02f, 0.02f);
  std::uniform_real_distribution<float> angleNoise(-0.01f, 0.01f);
  std::uniform_real_distribution<float> speedNoise(0.95f, 1.05f);

  int raysPerEdge = count / 4;
  int remainder = count % 4;

  // Left edge - moving right
  for (int i = 0; i < raysPerEdge + (remainder > 0 ? 1 : 0); i++) {
    float spacing = 4.0f / (raysPerEdge + 1);
    float y = -2.0f + spacing * (i + 1) + posNoise(gen);
    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(-2.0f, y),
      raySpeed * speedNoise(gen),
      segmentCount,
      0.0f + angleNoise(gen)
    ));
  }
  if (remainder > 0) remainder--;

  // Right edge - moving left
  for (int i = 0; i < raysPerEdge + (remainder > 0 ? 1 : 0); i++) {
    float spacing = 4.0f / (raysPerEdge + 1);
    float y = -2.0f + spacing * (i + 1) + posNoise(gen);
    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(2.0f, y),
      raySpeed * speedNoise(gen),
      segmentCount,
      M_PI + angleNoise(gen)
    ));
  }
  if (remainder > 0) remainder--;

  // Top edge - moving down
  for (int i = 0; i < raysPerEdge + (remainder > 0 ? 1 : 0); i++) {
    float spacing = 4.0f / (raysPerEdge + 1);
    float x = -2.0f + spacing * (i + 1) + posNoise(gen);
    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, 2.0f),
      raySpeed * speedNoise(gen),
      segmentCount,
      -M_PI / 2 + angleNoise(gen)
    ));
  }
  if (remainder > 0) remainder--;

  // Bottom edge - moving up
  for (int i = 0; i < raysPerEdge; i++) {
    float spacing = 4.0f / (raysPerEdge + 1);
    float x = -2.0f + spacing * (i + 1) + posNoise(gen);
    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, -2.0f),
      raySpeed * speedNoise(gen),
      segmentCount,
      M_PI / 2 + angleNoise(gen)
    ));
  }

  return rays;
}

// RadialSpawnStrategy Implementation
std::vector<std::unique_ptr<LightRay>> RadialSpawnStrategy::CreateRayBatch(
  int count,
  float raySpeed,
  int segmentCount) {

  std::vector<std::unique_ptr<LightRay>> rays;
  rays.reserve(count);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> radiusNoise(0.95f, 1.05f);
  std::uniform_real_distribution<float> angleNoise(-0.02f, 0.02f);
  std::uniform_real_distribution<float> speedNoise(0.95f, 1.05f);

  for (int i = 0; i < count; i++) {
    float angle = (2.0f * M_PI * i) / count;
    float r = spawnRadius * radiusNoise(gen);

    float x = r * cos(angle);
    float y = r * sin(angle);

    // Point rays toward center with slight variation
    float targetAngle = angle + M_PI + angleNoise(gen);

    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, y),
      raySpeed * speedNoise(gen),
      segmentCount,
      targetAngle
    ));
  }

  return rays;
}

// SpiralSpawnStrategy Implementation
std::vector<std::unique_ptr<LightRay>> SpiralSpawnStrategy::CreateRayBatch(
  int count,
  float raySpeed,
  int segmentCount) {

  std::vector<std::unique_ptr<LightRay>> rays;
  rays.reserve(count);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> speedNoise(0.95f, 1.05f);
  std::uniform_real_distribution<float> angleNoise(-0.02f, 0.02f);

  for (int i = 0; i < count; i++) {
    // Interpolate radius from start to end
    float t = (float)i / (float)count;
    float radius = radiusStart + (radiusEnd - radiusStart) * t;

    float x = radius * cos(currentAngle);
    float y = radius * sin(currentAngle);

    // Point rays toward center
    float targetAngle = currentAngle + M_PI + angleNoise(gen);

    rays.push_back(std::make_unique<LightRay>(
      glm::vec2(x, y),
      raySpeed * speedNoise(gen),
      segmentCount,
      targetAngle
    ));

    // Advance angle for spiral
    currentAngle += angleIncrement;
    if (currentAngle > 2 * M_PI) {
      currentAngle -= 2 * M_PI;
    }
  }

  return rays;
}

// RayFactory Implementation
RayFactory::RayFactory() {
  // Default to left edge strategy
  SetSpawnStrategy(SpawnPattern::LEFT_EDGE);
}

void RayFactory::SetSpawnStrategy(SpawnPattern pattern) {
  switch (pattern) {
  case SpawnPattern::LEFT_EDGE:
    currentStrategy = std::make_unique<LeftEdgeSpawnStrategy>();
    break;
  case SpawnPattern::FOUR_EDGES:
    currentStrategy = std::make_unique<FourEdgeSpawnStrategy>();
    break;
  case SpawnPattern::RADIAL:
    currentStrategy = std::make_unique<RadialSpawnStrategy>();
    break;
  case SpawnPattern::SPIRAL:
    currentStrategy = std::make_unique<SpiralSpawnStrategy>();
    break;
  }
}

void RayFactory::SetSpawnStrategy(std::unique_ptr<IRaySpawnStrategy> strategy) {
  currentStrategy = std::move(strategy);
}

std::vector<std::unique_ptr<LightRay>> RayFactory::CreateRays(
  int count,
  float raySpeed,
  int segmentCount) {

  if (!currentStrategy) {
    // Fallback to default if no strategy set
    SetSpawnStrategy(SpawnPattern::LEFT_EDGE);
  }

  return currentStrategy->CreateRayBatch(count, raySpeed, segmentCount);
}

const char* RayFactory::GetCurrentStrategyName() const {
  if (currentStrategy) {
    return currentStrategy->GetStrategyName();
  }
  return "None";
}