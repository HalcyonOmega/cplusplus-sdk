# Testing/CMakeLists.txt
project(MCP-Tests)

# Find necessary packages
find_package(Poco REQUIRED COMPONENTS Net JSON Foundation)
find_package(Threads REQUIRED)

# Add the test executable
add_executable(mcp_tests
    TestMain.cpp
    TestTransportHTTP.cpp
    TestTransportSTDIO.cpp
    ${CMAKE_SOURCE_DIR}/Source/Public/Utilities/JSON/JSONMessages.cpp
)

# Add include directories to find SDK headers
target_include_directories(mcp_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source/Public
)

# Link against required libraries
target_link_libraries(mcp_tests PRIVATE
    Poco::Net
    Poco::JSON
    Poco::Foundation
    Threads::Threads
) 