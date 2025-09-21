#include "LightRay.h"
#include <algorithm>
#include <cmath>
#include <iostream>


// Static member definitions with default values
float LightRay::gravityMultiplier = 1.0f;    // Default: normal gravity
float LightRay::maxForce = 15.0f;            // Default: cap at 15
float LightRay::forceExponent = 2.0f;        // Default: 1/r² falloff
float LightRay::minDistance = 0.001f;        // Default: very small minimum


LightRay::LightRay(float startY, float speed, int segmentCount, float angle)
  : startY(startY)
  , baseSpeed(speed)
  , initialAngle(angle)
  , absorbed(false)
  , maxSegments(segmentCount * 10) {  // Much longer rays (10x)
  Reset();
}


void LightRay::Reset() {
  absorbed = false;
  segments.clear();

  // Initialize ray starting from left edge
  headPosition = glm::vec2(-1.0f, startY);

  // Set initial velocity straight to the right
  headVelocity = glm::vec2(baseSpeed, 0.0f);

  // Create initial trail extending off-screen to the left
  // This represents the ray that was emitted before entering the screen
  float segmentLength = 0.02f;

  for (int i = 0; i < 50; ++i) {  // Shorter initial trail since we'll build it up
    float x = headPosition.x - i * segmentLength;
    float y = headPosition.y;  // Straight line initially
    segments.push_back(glm::vec2(x, y));
  }
}


bool LightRay::IsOrbiting() const {
  // Check if ray is in a roughly circular path
  // This happens when velocity is mostly perpendicular to position vector
  if (segments.size() < 10) return false;

  // Check if recent positions form a curve
  glm::vec2 center = glm::vec2(0.5f, 0.0f); // Approximate black hole position
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
  // F = G * M * multiplier / r^exponent
  // Default exponent = 2 gives standard 1/r² gravity
  // Exponent = 1 gives 1/r (stronger at distance)
  // Exponent = 3 gives 1/r³ (weaker at distance, stronger close)
  float forceMagnitude = blackholeMass * gravityMultiplier / pow(distance, forceExponent);

  // Apply configurable force cap
  float originalForce = forceMagnitude;
  forceMagnitude = std::min(forceMagnitude, maxForce);

  // Debug output (only occasionally)
  //static int printCounter = 0;
  //if (++printCounter % 500 == 0) {
  //  std::cout << "Gravity: Dist=" << distance
  //    << ", Force=" << originalForce
  //    << " (capped: " << forceMagnitude << ")"
  //    << ", Mult=" << gravityMultiplier
  //    << ", Exp=" << forceExponent << std::endl;
  //}

  // Return force vector
  return glm::normalize(toBlackhole) * forceMagnitude;
}

void LightRay::PropagateRay(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon) {
  // Only the ray HEAD is affected by gravity
  // Existing segments (photons that already passed) remain fixed in space

  // Calculate gravitational force on the ray head only
  glm::vec2 force = CalculateGravitationalForce(headPosition, blackholePos, blackholeMass);

  // Update velocity of the head (F = ma, assume m = 1)
  headVelocity += force * deltaTime;

  // Maintain constant speed (speed of light is constant)
  float currentSpeed = glm::length(headVelocity);
  float targetSpeed = baseSpeed;

  // Normalize to maintain constant speed
  if (currentSpeed > 0.001f) {
    headVelocity = glm::normalize(headVelocity) * targetSpeed;
  }

  // Update head position only
  headPosition += headVelocity * deltaTime;

  // Check if ray head hit the event horizon
  float distToBlackhole = glm::length(headPosition - blackholePos);
  if (distToBlackhole < eventHorizon) {
    absorbed = true;
    // Freeze the head at the event horizon edge
    glm::vec2 toCenter = blackholePos - headPosition;
    headPosition = blackholePos - glm::normalize(toCenter) * eventHorizon;
    // Slow down dramatically (time dilation effect)
    headVelocity *= 0.1f;
  }
}



void LightRay::UpdateSegments(float deltaTime) {
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

  // IMPORTANT: Do NOT modify existing segments!
  // Once a photon has passed through a point in space, that part of the ray
  // should remain fixed. We only add new segments at the head.

  // Trim the tail if ray is too long (for memory management)
  if (segments.size() > maxSegments) {
    segments.resize(maxSegments);
  }

  // That's it! We don't move existing segments because light that has
  // already traveled through space doesn't change position.
}



void LightRay::Update(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon) {
  // Continue updating even if absorbed - rays freeze at event horizon
  // if (absorbed) return;  // REMOVED - keep updating

  // Update the ray head position based on physics
  PropagateRay(deltaTime, blackholePos, blackholeMass, eventHorizon);

  // Update the ray segments to follow the head
  UpdateSegments(deltaTime);

  // Check if ray needs reset (went off screen)
  if (NeedsReset()) {
    Reset();
  }
}


bool LightRay::NeedsReset() const {
  if (segments.empty()) return true;

  // Check if the ray head has gone off screen
  if (headPosition.x > 1.5f || headPosition.x < -1.5f ||
    headPosition.y > 1.5f || headPosition.y < -1.5f) {

    // Only reset if moving away from screen (not orbiting back)
    if (headPosition.x > 1.2f && headVelocity.x > 0) {
      return true;
    }

    // Check if entire ray has left the visible area
    bool anyVisible = false;
    for (size_t i = 0; i < std::min(size_t(20), segments.size()); ++i) {
      const auto& segment = segments[i];
      if (segment.x >= -1.2f && segment.x <= 1.2f &&
        segment.y >= -1.2f && segment.y <= 1.2f) {
        anyVisible = true;
        break;
      }
    }

    if (!anyVisible) {
      return true;
    }
  }

  return false;
}