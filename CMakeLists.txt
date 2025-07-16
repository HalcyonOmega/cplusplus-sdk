# CMakeLists.txt
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
option(BUILD_EXAMPLES "Build examples" ON)

# Compiler-specific options
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
            -Wextra
            -Wpedantic
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
            /W4
            /WX
    )
endif ()

# Include directories
include_directories(Source/Public)
include_directories(Source/Public/Proxies)
include_directories(Source/Public/Utilities/ThirdParty)

# Add testing if enabled
if (BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Testing/CMakeLists.txt)
    enable_testing()
    add_subdirectory(Testing)
endif ()

# Add examples if enabled
if (BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Examples/CMakeLists.txt)
    add_subdirectory(Examples)
endif ()


# Gather all source and header files for the library
file(GLOB_RECURSE SDK_SOURCES
        "Source/Private/**/*.cpp"
        "Source/Public/**/*.cpp"
)
file(GLOB_RECURSE SDK_HEADERS
        "Source/Public/**/*.h"
        "Source/Public/**/*.hpp"
)

# Print the files being included in the build
message(STATUS "Source files:")
foreach (SOURCE ${SDK_SOURCES})
    message(STATUS "  ${SOURCE}")
endforeach ()

message(STATUS "Header files:")
foreach (HEADER ${SDK_HEADERS})
    message(STATUS "  ${HEADER}")
endforeach ()

# Create the SDK library
add_library(${PROJECT_NAME} STATIC ${SDK_SOURCES} ${SDK_HEADERS})

# Set C++ standard for the library
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD ${CMAKE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
)

# Public include directories for the library
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/Public>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Private
)


find_package(Poco REQUIRED COMPONENTS Foundation Net)

target_link_libraries(${PROJECT_NAME} PUBLIC Poco::Foundation Poco::Net)

# On Windows, Poco::Net requires the system networking library.
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib iphlpapi.lib)
endif ()


add_executable(HTTPTest Source/Public/T_HTTPServer.cpp)

target_link_libraries(HTTPTest PUBLIC ${PROJECT_NAME}})