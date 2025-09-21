#include "BlackholeApp.h"
#include <iostream>
#include <chrono>

int main() {
  // Create the black hole simulation app
  BlackholeApp app(1024, 768);

  // Initialize the application
  if (!app.Initialize()) {
    std::cerr << "Failed to initialize BlackholeApp" << std::endl;
    return -1;
  }


  std::cout << "==================================" << std::endl;
  std::cout << "Black Hole Light Ray Simulation" << std::endl;
  std::cout << "==================================" << std::endl;
  std::cout << "Movement Controls:" << std::endl;
  std::cout << "  Arrow Keys: Move black hole position" << std::endl;
  std::cout << std::endl;
  std::cout << "Physics Controls:" << std::endl;
  std::cout << "  Q/E: Decrease/Increase black hole MASS" << std::endl;
  std::cout << "  Z/X: Decrease/Increase black hole RADIUS" << std::endl;
  std::cout << "  A/S: Decrease/Increase LIGHT SPEED" << std::endl;
  std::cout << std::endl;
  std::cout << "Advanced Gravity Controls:" << std::endl;
  std::cout << "  D/F: Decrease/Increase GRAVITY MULTIPLIER (accel/decel effect)" << std::endl;
  std::cout << "  C/V: Decrease/Increase MAX FORCE CAP" << std::endl;
  std::cout << "  G/H: Decrease/Increase FORCE EXPONENT (falloff curve)" << std::endl;
  std::cout << std::endl;
  std::cout << "Other Controls:" << std::endl;
  std::cout << "  SPACE or R: Reset simulation (keeps settings)" << std::endl;
  std::cout << "  P: Print current parameters" << std::endl;
  std::cout << "  ESC: Exit" << std::endl;
  std::cout << "==================================" << std::endl;

  // Timing
  auto lastTime = std::chrono::high_resolution_clock::now();

  // Main loop
  while (!app.ShouldClose()) {
    // Calculate delta time
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    // Process input
    app.ProcessInput(app.GetWindow());

    // Update simulation
    app.Update(deltaTime);

    // Render
    app.Render();
  }

  std::cout << "Simulation Ended" << std::endl;
  return 0;
}