# C++MCP SDK

A C++ implementation of the MCP (Message Control Protocol) SDK.

## Dependencies

### Third-Party Libraries

#### cpp-httplib
- **Version**: Latest (from master branch)
- **License**: MIT
- **Source**: https://github.com/yhirose/cpp-httplib
- **Purpose**: HTTP client/server functionality for the StreamableHTTPTransport
- **Location**: `Source/ThirdParty/httplib.h`

#### nlohmann/json
- **Version**: Latest (from develop branch)
- **License**: MIT
- **Source**: https://github.com/nlohmann/json
- **Purpose**: JSON parsing and serialization
- **Location**: `Source/ThirdParty/json.hpp`

## Building

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler
- Git

### Build Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/mcp-sdk.git
   cd mcp-sdk
   ```

2. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure and build:
   ```bash
   cmake ..
   make
   ```

4. Install (optional):
   ```bash
   make install
   ```

## Usage

### Basic Example
```cpp
#include <mcp_sdk/Communication/Transport/StreamableHTTPTransport.h>

int main() {
    // Create a transport
    MCP::StreamableHTTPTransport transport("http://example.com/events");

    // Set up callbacks
    transport.SetOnMessage([](const std::string& message, const MCP::AuthInfo* auth) {
        std::cout << "Received message: " << message << std::endl;
    });

    transport.SetOnError([](const std::string& error) {
        std::cerr << "Error: " << error << std::endl;
    });

    // Start the transport
    transport.Start();

    // Send a message
    transport.Send("Hello, World!");

    // Stop the transport
    transport.Stop();

    return 0;
}
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.
