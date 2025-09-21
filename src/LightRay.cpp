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
const float LightRay::ABSORPTION_RESPAWN_TIME = 0.1f; // Respawn after 0.1 seconds

// Constructor for radial rays
LightRay::LightRay(glm::vec2 startPos, float speed, int segmentCount, float angle)
  : startPosition(startPos)
  , baseSpeed(speed)
  , initialAngle(angle)
  , absorbed(false)
  , maxSegments(segmentCount * 10)
  , timeSinceAbsorption(0.0f) {
  Reset();
}

// Reset method for radial rays
void LightRay::Reset() {
  absorbed = false;
  timeSinceAbsorption = 0.0f;
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

  // Create initial trail extending backwards from start position
  float segmentLength = 0.02f;

  for (int i = 0; i < 50; ++i) {
    float x = headPosition.x - i * segmentLength * cos(finalAngle);
    float y = headPosition.y - i * segmentLength * sin(finalAngle);
    segments.push_back(glm::vec2(x, y));
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

glm::vec2 LightRay::CalculateGravitationalForce(glm::vec2 position, glm::vec2 blackholePos, float blackholeMass) {
  // Calculate vector from position to black hole
  glm::vec2 toBlackhole = blackholePos - position;
  float distance = glm::length(toBlackhole);

  // Prevent division by zero with configurable minimum
  if (distance < minDistance) distance = minDistance;

  // Gravitational force with configurable falloff exponent
  float forceMagnitude = blackholeMass * gravityMultiplier / pow(distance, forceExponent);

  // Add orbital boost zone - helps create stable orbits
  // Sweet spot at around 2-3x the event horizon radius
  float optimalOrbitRadius = 0.6f;  // Tune this for orbital distance
  if (distance > optimalOrbitRadius * 0.7f && distance < optimalOrbitRadius * 1.3f) {
    forceMagnitude *= 1.2f;  // Boost force in orbital zone to help capture rays
  }

  // Apply configurable force cap
  forceMagnitude = std::min(forceMagnitude, maxForce);

  // Return force vector
  return glm::normalize(toBlackhole) * forceMagnitude;
}

void LightRay::PropagateRay(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon) {
  // Only the ray HEAD is affected by gravity
  // Existing segments (photons that already passed) remain fixed in space

  // If absorbed, update absorption timer but don't move the ray
  if (absorbed) {
    timeSinceAbsorption += deltaTime;
    return;
  }

  // Calculate gravitational force on the ray head only
  glm::vec2 force = CalculateGravitationalForce(headPosition, blackholePos, blackholeMass);

  // Update velocity of the head (F = ma, assume m = 1)
  headVelocity += force * deltaTime;

  // Maintain constant speed (speed of light is constant)
  float currentSpeed = glm::length(headVelocity);

  // Normalize to maintain constant speed
  if (currentSpeed > 0.001f) {
    headVelocity = glm::normalize(headVelocity) * baseSpeed;
  }

  // Update head position
  headPosition += headVelocity * deltaTime;

  // Check if ray head hit the event horizon
  float distToBlackhole = glm::length(headPosition - blackholePos);
  if (distToBlackhole < eventHorizon) {
    absorbed = true;
    timeSinceAbsorption = 0.0f;
    // Freeze the head at the event horizon edge
    glm::vec2 toCenter = blackholePos - headPosition;
    headPosition = blackholePos - glm::normalize(toCenter) * eventHorizon;
    // Slow down dramatically (time dilation effect)
    headVelocity *= 0.1f;
  }
}

void LightRay::UpdateSegments(float deltaTime) {
  // Don't update segments if absorbed (ray is frozen at event horizon)
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

bool LightRay::ShouldRespawn() const {
  // Respawn if absorbed for too long
  return absorbed && timeSinceAbsorption > ABSORPTION_RESPAWN_TIME;
}

// NeedsReset for radial pattern
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