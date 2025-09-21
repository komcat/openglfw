// Updated LightRay.cpp with more accurate physics
#include "LightRay.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

// Static member definitions
float LightRay::gravityMultiplier = 1.0f;
float LightRay::maxForce = 15.0f;
float LightRay::forceExponent = 2.0f;
float LightRay::minDistance = 0.001f;
const float LightRay::ABSORPTION_RESPAWN_TIME = 0.1f;

// Constructor for radial rays
LightRay::LightRay(glm::vec2 startPos, float speed, int segmentCount, float angle)
  : startPosition(startPos)
  , baseSpeed(speed)
  , initialAngle(angle)
  , absorbed(false)
  , maxSegments(segmentCount * 10)
  , timeSinceAbsorption(0.0f)
  , properTime(0.0f)  // Add this new member
  , angularMomentum(0.0f) {  // Add this new member
  Reset();
}

// Reset method for radial rays
void LightRay::Reset() {
  absorbed = false;
  timeSinceAbsorption = 0.0f;
  properTime = 0.0f;
  segments.clear();

  // Add some randomization for variety
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_real_distribution<float> posNoise(-0.02f, 0.02f);
  static std::uniform_real_distribution<float> angleNoise(-0.03f, 0.03f);

  // Initialize ray at starting position with slight noise
  headPosition = startPosition + glm::vec2(posNoise(gen), posNoise(gen));

  // Set initial velocity based on angle (with slight variation)
  float finalAngle = initialAngle + angleNoise(gen);
  float vx = baseSpeed * cos(finalAngle);
  float vy = baseSpeed * sin(finalAngle);
  headVelocity = glm::vec2(vx, vy);

  // Calculate angular momentum (conserved quantity in GR)
  // L = r × v (for 2D, this gives us the z-component)
  angularMomentum = headPosition.x * headVelocity.y - headPosition.y * headVelocity.x;

  // Create initial trail extending backwards from start position
  float segmentLength = 0.02f;

  for (int i = 0; i < 50; ++i) {
    float x = headPosition.x - i * segmentLength * cos(finalAngle);
    float y = headPosition.y - i * segmentLength * sin(finalAngle);
    segments.push_back(glm::vec2(x, y));
  }
}

// New method: Calculate deflection based on simplified GR equations
glm::vec2 LightRay::CalculateGeodesicDeflection(glm::vec2 position, glm::vec2 velocity,
  glm::vec2 blackholePos, float blackholeMass) {
  // Vector from position to black hole
  glm::vec2 toBlackhole = blackholePos - position;
  float r = glm::length(toBlackhole);

  // Prevent singularity
  if (r < minDistance) r = minDistance;

  // Schwarzschild radius (in our units)
  float rs = 2.0f * blackholeMass;  // Simplified: rs = 2GM/c² where G=c=1 in our units

  // Too close to black hole - about to be absorbed
  if (r < rs * 0.5f) {
    // Strong field regime - use approximation
    return glm::normalize(toBlackhole) * maxForce;
  }

  // Calculate perpendicular and radial components
  glm::vec2 rHat = toBlackhole / r;  // Unit vector toward black hole

  // Perpendicular unit vector (rotated 90 degrees)
  glm::vec2 phiHat(-rHat.y, rHat.x);

  // For a Schwarzschild metric, the acceleration components are:
  // a_r = -(rs/2r²)(1 - rs/r) for radial
  // a_φ = -(rs/r³)L where L is angular momentum

  float radialAccel = -(rs / (2.0f * r * r)) * (1.0f - rs / r);
  float tangentialAccel = -(rs / (r * r * r)) * std::abs(angularMomentum) * 0.1f; // Scaled for visibility

  // Combine components
  glm::vec2 acceleration = radialAccel * rHat + tangentialAccel * phiHat;

  // Apply multipliers for tuning
  acceleration *= gravityMultiplier;

  // Cap the maximum acceleration
  float accelMagnitude = glm::length(acceleration);
  if (accelMagnitude > maxForce) {
    acceleration = (acceleration / accelMagnitude) * maxForce;
  }

  return acceleration;
}

// New method: Calculate time dilation factor
float LightRay::CalculateTimeDilation(float r, float blackholeMass) {
  float rs = 2.0f * blackholeMass;  // Schwarzschild radius

  // Prevent division by zero or negative values
  if (r <= rs) return 0.01f;  // Nearly frozen at event horizon

  // Time dilation factor: dt/dτ = 1/√(1 - rs/r)
  float factor = 1.0f / sqrt(1.0f - rs / r);

  // Clamp to reasonable values
  return std::min(factor, 10.0f);
}

void LightRay::PropagateRay(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon) {
  // If absorbed, update absorption timer but don't move the ray
  if (absorbed) {
    timeSinceAbsorption += deltaTime;
    return;
  }

  // Calculate distance to black hole
  float r = glm::length(headPosition - blackholePos);

  // Calculate time dilation
  float timeDilationFactor = CalculateTimeDilation(r, blackholeMass);

  // Effective time step (proper time)
  float effectiveDeltaTime = deltaTime / timeDilationFactor;
  properTime += effectiveDeltaTime;

  // PHYSICS UPDATE 1: Use geodesic equations instead of simple force
  glm::vec2 acceleration = CalculateGeodesicDeflection(headPosition, headVelocity,
    blackholePos, blackholeMass);

  // Update velocity (only direction changes, not speed!)
  glm::vec2 newVelocity = headVelocity + acceleration * effectiveDeltaTime;

  // PHYSICS UPDATE 2: Maintain constant light speed
  float currentSpeed = glm::length(newVelocity);
  if (currentSpeed > 0.001f) {
    headVelocity = glm::normalize(newVelocity) * baseSpeed;  // Always travels at baseSpeed
  }

  // PHYSICS UPDATE 3: Position update includes time dilation
  // Near the black hole, coordinate time passes slower
  headPosition += headVelocity * effectiveDeltaTime;

  // Update angular momentum (should be conserved, but recalculate for numerical stability)
  angularMomentum = headPosition.x * headVelocity.y - headPosition.y * headVelocity.x;

  // Check if ray hit the event horizon
  if (r < eventHorizon) {
    absorbed = true;
    timeSinceAbsorption = 0.0f;
    // Freeze at event horizon
    glm::vec2 toCenter = blackholePos - headPosition;
    headPosition = blackholePos - glm::normalize(toCenter) * eventHorizon;
    // Note: In real physics, we'd never see it reach the horizon due to infinite time dilation
  }
}

void LightRay::UpdateSegments(float deltaTime) {
  // Don't update segments if absorbed (frozen at event horizon)
  if (absorbed) {
    return;
  }

  // Add new head position to the front of segments
  if (!segments.empty()) {
    // Only add if moved enough distance from last segment
    float distFromLast = glm::length(headPosition - segments[0]);
    if (distFromLast > 0.01f) {  // Minimum distance between segments
      segments.insert(segments.begin(), headPosition);
    }
  }
  else {
    segments.push_back(headPosition);
  }

  // Trim the tail if ray is too long (for memory management)
  if (segments.size() > maxSegments) {
    segments.resize(maxSegments);
  }
}

void LightRay::Update(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon) {
  // Continue updating even if absorbed
  PropagateRay(deltaTime, blackholePos, blackholeMass, eventHorizon);
  UpdateSegments(deltaTime);

  // Check if ray needs reset
  if (NeedsReset()) {
    Reset();
  }
  // Check if absorbed ray should respawn
  else if (ShouldRespawn()) {
    Reset();
  }
}

bool LightRay::IsOrbiting() const {
  // Check if ray is in a roughly circular path
  if (segments.size() < 10) return false;

  // Check if recent positions form a curve around origin (0,0)
  glm::vec2 center = glm::vec2(0.0f, 0.0f); // Black hole at origin
  float avgRadius = 0;
  for (size_t i = 0; i < std::min(size_t(10), segments.size()); ++i) {
    avgRadius += glm::length(segments[i] - center);
  }
  avgRadius /= std::min(size_t(10), segments.size());

  // Check variance in radius (low variance = circular orbit)
  float variance = 0;
  for (size_t i = 0; i < std::min(size_t(10), segments.size()); ++i) {
    float r = glm::length(segments[i] - center);
    variance += (r - avgRadius) * (r - avgRadius);
  }
  variance /= std::min(size_t(10), segments.size());

  return variance < 0.01f && avgRadius < 0.5f;  // Small variance and close to black hole
}

bool LightRay::ShouldRespawn() const {
  // Respawn if absorbed for too long
  return absorbed && timeSinceAbsorption > ABSORPTION_RESPAWN_TIME;
}

bool LightRay::NeedsReset() const {
  if (segments.empty()) return true;

  // Don't reset absorbed rays based on position (they'll reset via ShouldRespawn)
  if (absorbed) {
    return false;
  }

  // Check if ray has traveled too far from center
  float distFromCenter = glm::length(headPosition);

  // Reset if ray has gone far off screen (>2.5 units from center)
  if (distFromCenter > 2.5f) {
    return true;
  }

  // Check visibility - at least some part should be visible
  bool anyVisible = false;
  float maxVisible = 2.0f;  // Extended visibility range for radial pattern

  for (size_t i = 0; i < std::min(size_t(20), segments.size()); ++i) {
    const auto& segment = segments[i];
    if (std::abs(segment.x) <= maxVisible && std::abs(segment.y) <= maxVisible) {
      anyVisible = true;
      break;
    }
  }

  return !anyVisible;
}