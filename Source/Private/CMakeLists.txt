# Source/Private/CMakeLists.txt

# Recursively find all source files in Private directory
file(GLOB_RECURSE PRIVATE_SOURCES
    "*.cpp"
    "*.cc" 
    "*.cxx"
)

file(GLOB_RECURSE PRIVATE_HEADERS
    "*.h"
    "*.hpp"
    "*.hxx"
)

# Add sources to the main library target
if(PRIVATE_SOURCES OR PRIVATE_HEADERS)
    target_sources(${PROJECT_NAME} PRIVATE ${PRIVATE_SOURCES} ${PRIVATE_HEADERS})
endif()

# Add include directories for private headers
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}) 