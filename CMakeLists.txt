cmake_minimum_required(VERSION 3.20)
project(cplusplus-mcp-sdk
        VERSION 1.0.0
        DESCRIPTION "C++ SDK for MCP (Model Context Protocol)"
        LANGUAGES CXX C
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTING "Build tests" ON)

# Compiler-specific options
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
            -Wextra
            -Wpedantic
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
    )
endif ()


# Add testing if enabled
if (BUILD_TESTING AND EXISTS SDK/Testing/CMakeLists.txt)
    enable_testing()
    add_subdirectory(SDK/Testing)
endif ()


# Gather all source and header files for the library
file(GLOB_RECURSE SDK_SOURCES
        "SDK/Source/Private/**/*.cpp"
        "SDK/Source/Public/**/*.cpp"
)
file(GLOB_RECURSE SDK_HEADERS
        "SDK/Source/Public/**/*.h"
        "SDK/Source/Public/**/*.hpp"
)

# Create the SDK library
add_library(${PROJECT_NAME} STATIC ${SDK_SOURCES} ${SDK_HEADERS})

# Set C++ standard for the library
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD_REQUIRED ON
        #COMPILE_FLAGS "-P"
)

# Public include directories for the library
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SDK/Source/Public>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SDK/Source/Public/Proxies>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SDK/Source/Public/Utilities/ThirdParty>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDK/Source/Private
)

# Poco Setup
set(Poco_STATIC ON)
find_package(Poco REQUIRED COMPONENTS Foundation Net Util)
target_link_libraries(${PROJECT_NAME} PUBLIC Poco::Foundation Poco::Net Poco::Util)

# On Windows, Poco::Net requires the system networking library.
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib iphlpapi.lib)
endif ()

# HTTP Test Executable
add_executable(HTTPTest SDK/Source/T_HTTPServer.cpp)

target_link_libraries(HTTPTest PUBLIC Poco::Foundation Poco::Net Poco::Util)
target_link_libraries(HTTPTest PUBLIC ${PROJECT_NAME})

if (MINGW)
    target_link_options(HTTPTest PRIVATE -static)
endif ()