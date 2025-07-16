# Examples/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Basic example executable
add_executable(basic_example
  BasicExample.cpp
)

target_link_libraries(basic_example
  PRIVATE
  cplusplus-mcp-sdk
)
