// Updated LightRay.h with more accurate physics
#pragma once

#include <glm/glm.hpp>
#include <vector>

class LightRay {
public:
  // Constructor that takes a starting position instead of just Y
  LightRay(glm::vec2 startPos, float speed = 0.3f, int segmentCount = 50, float angle = 0.0f);

  // Reset the ray to starting position
  void Reset();

  // Update the ray physics
  void Update(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon);

  // Get the ray segments for rendering (as a continuous line)
  const std::vector<glm::vec2>& GetSegments() const { return segments; }

  // Check if ray is absorbed
  bool IsAbsorbed() const { return absorbed; }

  // Check if ray needs reset (went off screen or absorbed)
  bool NeedsReset() const;

  // Check if ray should respawn (absorbed for too long)
  bool ShouldRespawn() const;

  // Set/Get properties
  void SetSpeed(float s) { baseSpeed = s; }
  float GetSpeed() const { return baseSpeed; }

  // Check if ray is orbiting
  bool IsOrbiting() const;

  // Get proper time (for time dilation effects)
  float GetProperTime() const { return properTime; }

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
  glm::vec2 startPosition;    // Full starting position
  float baseSpeed;             // Base speed (speed of light)
  float initialAngle;          // Initial launch angle
  bool absorbed;               // Has the ray been absorbed?

  // Ray segments (the continuous beam)
  std::vector<glm::vec2> segments;    // Current ray segments forming the beam
  int maxSegments;                     // Maximum number of segments

  // Physics state for the leading edge of the ray
  glm::vec2 headPosition;      // Current position of ray head
  glm::vec2 headVelocity;      // Current velocity of ray head
  float angularMomentum;       // Conserved angular momentum (NEW)
  float properTime;            // Proper time along ray's path (NEW)

  // Absorption tracking
  float timeSinceAbsorption;   // Time since ray was absorbed
  static const float ABSORPTION_RESPAWN_TIME; // Time before respawning absorbed ray

  // Helper methods
  glm::vec2 CalculateGravitationalForce(glm::vec2 position, glm::vec2 blackholePos, float blackholeMass);
  glm::vec2 CalculateGeodesicDeflection(glm::vec2 position, glm::vec2 velocity,
    glm::vec2 blackholePos, float blackholeMass);  // NEW
  float CalculateTimeDilation(float r, float blackholeMass);  // NEW
  void UpdateSegments(float deltaTime);
  void PropagateRay(float deltaTime, glm::vec2 blackholePos, float blackholeMass, float eventHorizon);

  // Gravity tuning parameters
  static float gravityMultiplier;
  static float maxForce;
  static float forceExponent;
  static float minDistance;
};