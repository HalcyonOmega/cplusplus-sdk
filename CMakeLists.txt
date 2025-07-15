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
option(BUILD_TESTING "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Set default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# Compiler-specific options
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
            -Wextra
            -Wpedantic
            $<$<CONFIG:Debug>:-g3>
            $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Release>:-DNDEBUG>
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
            /W4
            /WX
            $<$<CONFIG:Debug>:/Od>
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Release>:/DNDEBUG>
    )
endif ()

# Find required packages
find_package(Threads REQUIRED)

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
        "Source/Private/*.cpp"
        "Source/Public/**/*.cpp"
)
file(GLOB_RECURSE SDK_HEADERS
        "Source/Public/**/*.h"
        "Source/Public/**/*.hpp"
)

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

# Link external dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)


find_package(Poco REQUIRED COMPONENTS Foundation Net)

target_link_libraries(${PROJECT_NAME} PRIVATE Poco::Foundation Poco::Net)

# On Windows, Poco::Net requires the system networking library.
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib iphlpapi.lib)
endif ()

# Installation rules
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY Source/Public/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)