# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(cplusplus-mcp-sdk 
    VERSION 1.0.0
    DESCRIPTION "C++ SDK for MCP (Model Context Protocol)"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(Threads REQUIRED)

# Gather all source and header files
file(GLOB_RECURSE ALL_SOURCES
    "${CMAKE_SOURCE_DIR}/Source/Private/*.cpp"
    "${CMAKE_SOURCE_DIR}/Source/Public/*.cpp"
)
file(GLOB_RECURSE ALL_HEADERS
    "${CMAKE_SOURCE_DIR}/Source/Private/*.h"
    "${CMAKE_SOURCE_DIR}/Source/Private/*.hpp"
    "${CMAKE_SOURCE_DIR}/Source/Public/*.h"
    "${CMAKE_SOURCE_DIR}/Source/Public/Core/Includes/*.h"
    "${CMAKE_SOURCE_DIR}/Source/Public/*.hpp"
)

# Create library
add_library(${PROJECT_NAME} STATIC ${ALL_SOURCES} ${ALL_HEADERS})

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Source/Public>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Source/ThirdParty>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source/Private
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    PUBLIC 
        Threads::Threads
        curl
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Public/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Install the export set
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
) 