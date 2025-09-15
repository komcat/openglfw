#include <glad/glad.h>  // MUST be included before GLFW!
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

// Vertex shader - applies rotation transformation
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor;
uniform float time;

void main()
{
    // Create rotation matrix
    float c = cos(time);
    float s = sin(time);
    mat3 rotation = mat3(
        c, -s, 0.0,
        s,  c, 0.0,
        0.0, 0.0, 1.0
    );
    
    vec3 rotatedPos = rotation * aPos;
    gl_Position = vec4(rotatedPos, 1.0);
    vertexColor = aColor;
}
)";

// Fragment shader - outputs interpolated colors
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

uniform float time;

void main()
{
    // Add some time-based color variation
    vec3 color = vertexColor * (0.5 + 0.5 * sin(time * 2.0));
    FragColor = vec4(color, 1.0);
}
)";

// Function to compile shaders
unsigned int compileShader(unsigned int type, const char* source)
{
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  // Check compilation
  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
  }

  return shader;
}

int main()
{
  // Initialize GLFW
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  // Configure GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

  // Create window
  GLFWwindow* window = glfwCreateWindow(1024, 768, "Test Window - Rotating Colorful Shape", NULL, NULL);
  if (!window)
  {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  // Make the window's context current
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable v-sync

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // Print OpenGL info
  std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
  std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
  std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
  std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;

  // Enable depth testing and multisampling
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  // Compile shaders
  unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

  // Create shader program
  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  // Check linking
  int success;
  char infoLog[512];
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
  }

  // Delete shaders as they're linked now
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Pentagon vertices with colors
  float vertices[] = {
    // positions         // colors
     0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // top - red
    -0.475f,  0.154f, 0.0f,  0.0f, 1.0f, 0.0f,  // top left - green
    -0.294f, -0.404f, 0.0f,  0.0f, 0.0f, 1.0f,  // bottom left - blue
     0.294f, -0.404f, 0.0f,  1.0f, 1.0f, 0.0f,  // bottom right - yellow
     0.475f,  0.154f, 0.0f,  1.0f, 0.0f, 1.0f,  // top right - magenta
  };

  unsigned int indices[] = {
      0, 1, 4,  // top triangle
      1, 2, 3,  // bottom left part
      1, 3, 4   // bottom right part
  };

  // Create VAO, VBO, and EBO
  unsigned int VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  // Bind VAO
  glBindVertexArray(VAO);

  // Setup VBO
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Setup EBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Color attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Unbind
  glBindVertexArray(0);

  // Get uniform location
  int timeLocation = glGetUniformLocation(shaderProgram, "time");

  // Wireframe mode toggle
  bool wireframe = false;
  bool spacePressed = false;

  std::cout << "\nControls:" << std::endl;
  std::cout << "  ESC   - Exit" << std::endl;
  std::cout << "  SPACE - Toggle wireframe mode" << std::endl;
  std::cout << "  R     - Reset rotation" << std::endl;

  // Render loop
  while (!glfwWindowShouldClose(window))
  {
    // Process input
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    // Toggle wireframe
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
      if (!spacePressed)
      {
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        spacePressed = true;
      }
    }
    else
    {
      spacePressed = false;
    }

    // Reset rotation
    static float timeOffset = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
      timeOffset = (float)glfwGetTime();
    }

    // Get window size for viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Clear buffers
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader program and set uniforms
    glUseProgram(shaderProgram);
    float currentTime = (float)glfwGetTime() - timeOffset;
    glUniform1f(timeLocation, currentTime);

    // Draw the shape
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteProgram(shaderProgram);

  std::cout << "Window closed, cleaning up..." << std::endl;
  glfwTerminate();
  return 0;
}