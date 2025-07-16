# Testing/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Add test executable
add_executable(sdk_tests
        ../Source/Public/T_HTTPServer.cpp
)

target_link_libraries(sdk_tests
        PRIVATE
        cplusplus-mcp-sdk
)

# Register the test
add_test(NAME sdk_tests COMMAND sdk_tests)
