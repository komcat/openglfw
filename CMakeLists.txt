# CMakeList.txt : CMake project for openglfw with GLAD
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("openglfw")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add GLFW
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/external/glfw")
set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
set(GLFW_LIB_DIR "${GLFW_DIR}/lib")

# Add GLAD
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/external/glad")
set(GLAD_INCLUDE_DIR "${GLAD_DIR}/include")
set(GLAD_SOURCE "${GLAD_DIR}/src/glad.c")

# Find OpenGL
find_package(OpenGL REQUIRED)

# Create GLAD library
add_library(glad STATIC ${GLAD_SOURCE})
target_include_directories(glad PUBLIC ${GLAD_INCLUDE_DIR})

# Create a variable for common include directories
set(COMMON_INCLUDES 
    ${GLFW_INCLUDE_DIR}
    ${GLAD_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIRS}
)

# Create a variable for common libraries
set(COMMON_LIBS
    glad  # GLAD must be linked before OpenGL
    ${OPENGL_LIBRARIES}
    "${GLFW_LIB_DIR}/glfw3.lib"
    # Windows system libraries
    $<$<PLATFORM_ID:Windows>:gdi32>
    $<$<PLATFORM_ID:Windows>:user32>
    $<$<PLATFORM_ID:Windows>:kernel32>
    $<$<PLATFORM_ID:Windows>:shell32>
)

# Add main executable
add_executable(openglfw "src/main.cpp")
target_include_directories(openglfw PRIVATE ${COMMON_INCLUDES})
target_link_libraries(openglfw ${COMMON_LIBS})

# Add tests subdirectory
add_subdirectory(tests)