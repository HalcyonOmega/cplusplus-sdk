# Source/Public/CMakeLists.txt

# Find all public headers
file(GLOB_RECURSE PUBLIC_HEADERS
    "*.h"
    "*.hpp" 
    "*.hxx"
)

# Find any public source files (if any)
file(GLOB_RECURSE PUBLIC_SOURCES
    "*.cpp"
    "*.cc"
    "*.cxx"
)

# Add sources to the main library target
if(PUBLIC_SOURCES)
    target_sources(${PROJECT_NAME} PRIVATE ${PUBLIC_SOURCES})
endif()

# Add headers as sources for IDE organization
if(PUBLIC_HEADERS)
    target_sources(${PROJECT_NAME} PRIVATE ${PUBLIC_HEADERS})
endif() 