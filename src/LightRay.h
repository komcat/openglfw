#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <deque>

class LightRay {
public:
  // Constructor with optional initial angle
  LightRay(float startY, float speed = 0.3f, int segmentCount = 50, float angle = 0.0f);

  // Reset the ray to starting position
  void Reset();

  // Update the ray physics
  void Update(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon);

  // Get the ray segments for rendering (as a continuous line)
  const std::vector<glm::vec2>& GetSegments() const { return segments; }

  // Check if ray is absorbed
  bool IsAbsorbed() const { return absorbed; }

  // Check if ray needs reset (went off screen)
  bool NeedsReset() const;

  // Set/Get properties
  void SetSpeed(float s) { baseSpeed = s; }
  float GetSpeed() const { return baseSpeed; }

  // Check if ray is orbiting
  bool IsOrbiting() const;

  // Static setters for global gravity parameters
  static void SetGravityMultiplier(float mult) { gravityMultiplier = mult; }
  static void SetMaxForce(float max) { maxForce = max; }
  static void SetForceExponent(float exp) { forceExponent = exp; }
  static void SetMinDistance(float min) { minDistance = min; }

  static float GetGravityMultiplier() { return gravityMultiplier; }
  static float GetMaxForce() { return maxForce; }
  static float GetForceExponent() { return forceExponent; }


private:
  // Ray properties
  float startY;           // Starting Y position (constant)
  float baseSpeed;        // Base speed (speed of light)
  float initialAngle;     // Initial launch angle
  bool absorbed;          // Has the ray been absorbed?

  // Ray segments (the continuous beam)
  std::vector<glm::vec2> segments;    // Current ray segments forming the beam
  int maxSegments;                     // Maximum number of segments

  // Physics state for the leading edge of the ray
  glm::vec2 headPosition;      // Current position of ray head
  glm::vec2 headVelocity;      // Current velocity of ray head

  // Helper methods
  glm::vec2 CalculateGravitationalForce(glm::vec2 position, glm::vec2 blackholePos, float blackholeMass);
  void UpdateSegments(float deltaTime);
  void PropagateRay(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon);

  // Gravity tuning parameters
  static float gravityMultiplier;     // Overall gravity strength multiplier
  static float maxForce;               // Maximum gravitational force cap
  static float forceExponent;          // Exponent for distance falloff (default 2 for 1/r²)
  static float minDistance;            // Minimum distance to prevent singularities


};