# CMakeLists.txt for tests directory

# Test executable for new window
add_executable(newwindow_test "newwindow.cpp")

# Include directories (using parent's variables)
target_include_directories(newwindow_test PRIVATE ${COMMON_INCLUDES})

# Link libraries (using parent's variables)
target_link_libraries(newwindow_test ${COMMON_LIBS})

# You can add more test executables here
# Example:
# add_executable(another_test "another_test.cpp")
# target_include_directories(another_test PRIVATE ${COMMON_INCLUDES})
# target_link_libraries(another_test ${COMMON_LIBS})

# If you want to add multiple test files into one executable:
# add_executable(combined_tests 
#     "test1.cpp"
#     "test2.cpp"
#     "test3.cpp"
# )
# target_include_directories(combined_tests PRIVATE ${COMMON_INCLUDES})
# target_link_libraries(combined_tests ${COMMON_LIBS})

# Optional: Set output directory for test executables
set_target_properties(newwindow_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

# If using DLL, copy it to test output directory
# if(WIN32)
#     add_custom_command(TARGET newwindow_test POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         "${GLFW_LIB_DIR}/glfw3.dll"
#         $<TARGET_FILE_DIR:newwindow_test>
#     )
# endif()