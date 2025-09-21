#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "LightRay.h"

// Abstract base class for ray spawning strategies
class IRaySpawnStrategy {
public:
  virtual ~IRaySpawnStrategy() = default;

  // Create a batch of rays
  virtual std::vector<std::unique_ptr<LightRay>> CreateRayBatch(
    int count,
    float raySpeed,
    int segmentCount = 10) = 0;

  // Get strategy name for debugging
  virtual const char* GetStrategyName() const = 0;
};

// Spawn rays from left edge moving right
class LeftEdgeSpawnStrategy : public IRaySpawnStrategy {
public:
  std::vector<std::unique_ptr<LightRay>> CreateRayBatch(
    int count,
    float raySpeed,
    int segmentCount) override;

  const char* GetStrategyName() const override { return "LeftEdge"; }
};

// Spawn rays from all four edges
class FourEdgeSpawnStrategy : public IRaySpawnStrategy {
public:
  std::vector<std::unique_ptr<LightRay>> CreateRayBatch(
    int count,
    float raySpeed,
    int segmentCount) override;

  const char* GetStrategyName() const override { return "FourEdge"; }
};

// Spawn rays in a radial pattern from outside
class RadialSpawnStrategy : public IRaySpawnStrategy {
private:
  float spawnRadius;

public:
  explicit RadialSpawnStrategy(float radius = 2.5f) : spawnRadius(radius) {}

  std::vector<std::unique_ptr<LightRay>> CreateRayBatch(
    int count,
    float raySpeed,
    int segmentCount) override;

  const char* GetStrategyName() const override { return "Radial"; }
};

// Spawn rays in a spiral pattern
class SpiralSpawnStrategy : public IRaySpawnStrategy {
private:
  float currentAngle;
  float angleIncrement;
  float radiusStart;
  float radiusEnd;

public:
  SpiralSpawnStrategy(float startRadius = 2.5f, float endRadius = 2.0f)
    : currentAngle(0.0f)
    , angleIncrement(0.1f)
    , radiusStart(startRadius)
    , radiusEnd(endRadius) {
  }

  std::vector<std::unique_ptr<LightRay>> CreateRayBatch(
    int count,
    float raySpeed,
    int segmentCount) override;

  const char* GetStrategyName() const override { return "Spiral"; }
};

// Factory class for creating rays
class RayFactory {
private:
  std::unique_ptr<IRaySpawnStrategy> currentStrategy;

public:
  enum class SpawnPattern {
    LEFT_EDGE,
    FOUR_EDGES,
    RADIAL,
    SPIRAL
  };

  RayFactory();

  // Set the spawn strategy
  void SetSpawnStrategy(SpawnPattern pattern);
  void SetSpawnStrategy(std::unique_ptr<IRaySpawnStrategy> strategy);

  // Create rays using current strategy
  std::vector<std::unique_ptr<LightRay>> CreateRays(
    int count,
    float raySpeed,
    int segmentCount = 10);

  // Get current strategy name
  const char* GetCurrentStrategyName() const;
};