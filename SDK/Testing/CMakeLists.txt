# Testing/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Add test executable
add_executable(sdk_tests
)

find_package(Poco REQUIRED COMPONENTS Foundation Net)

target_link_libraries(sdk_tests
        PRIVATE
        cplusplus-mcp-sdk
        Poco::Foundation
        Poco::Net
)

# Register the test
add_test(NAME sdk_tests COMMAND sdk_tests)
