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

# Build options
option(BUILD_TESTING "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-DNDEBUG>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
        /W4
        /WX
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/DNDEBUG>
    )
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(Source/Public)
include_directories(Source/Public/Core/Includes)
include_directories(Source/Public/ThirdParty)

# Add testing if enabled
if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Testing/CMakeLists.txt)
    enable_testing()
    add_subdirectory(Testing)
endif()

# Add examples if enabled
if(BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Examples/CMakeLists.txt)
    add_subdirectory(Examples)
endif()

### Old Code ###
# Installation
include(GNUInstallDirs)

# Install headers
install(DIRECTORY Source/Public/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

# Install third-party licenses
install(FILES THIRD_PARTY_LICENSES.md 
    DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Install documentation
install(DIRECTORY Docs/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    OPTIONAL)


# Create package config files (only if Config.cmake.in exists)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in)
    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )
endif()
### End Old Code ###

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
    "${CMAKE_SOURCE_DIR}/Source/Public/Core/Constants/*.h"
    "${CMAKE_SOURCE_DIR}/Source/Public/*.hpp"
    "${CMAKE_SOURCE_DIR}/Source/Public/ThirdParty/*.h"
    "${CMAKE_SOURCE_DIR}/Source/Public/ThirdParty/*.hpp"
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
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Source/Public/ThirdParty>
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