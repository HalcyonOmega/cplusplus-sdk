# Testing/CMakeLists.txt

# Find testing framework (you can change this to your preferred framework)
# For now, just using basic CTest functionality

# Function to add a test easily
function(add_unit_test test_name)
    add_executable(${test_name} ${ARGN})
    
    # Link against main library
    target_link_libraries(${test_name}
        PRIVATE
            ${PROJECT_NAME}::${PROJECT_NAME}
    )
    
    # Add to CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Add test subdirectories if they exist
set(TEST_DIRECTORIES
    Auth
    Client
    Communication
    Core
    Server
    Utilities
)

foreach(dir ${TEST_DIRECTORIES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
        message(STATUS "Adding test directory: ${dir}")
        # For now, just note that directories exist
        # You can add actual test files later
    endif()
endforeach()

# Example: Add a basic smoke test
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/basic_test.cpp)
    add_unit_test(basic_test basic_test.cpp)
endif()

# Add more specific tests as you create them
# add_unit_test(auth_test Auth/test_auth.cpp)
# add_unit_test(client_test Client/test_client.cpp)
# etc. 