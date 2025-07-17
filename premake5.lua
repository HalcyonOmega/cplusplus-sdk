-- Define the workspace
workspace "cplusplus-mcp-sdk"
architecture "x64"
configurations { "Debug", "Release" }

-- Define the main SDK library project
project "cplusplus-mcp-sdk"
location "C++_MCP_SDK"
kind "StaticLib"
language "C++"
cppdialect "C++20"
targetdir "bin/%{cfg.buildcfg}"
objdir "bin-obj/%{cfg.buildcfg}"

-- Project version and description
-- version "1.0.0"
-- description "C++ SDK for MCP (Model Context Protocol)"

-- Source and header files
files {
    "Source/**.h",
    "Source/**.cpp",
    "Source/**.hpp"
}

-- Include directories
includedirs {
    "Source/Public",
    "Source/Public/Proxies",
    "Source/Public/Utilities/ThirdParty",
    "Source/Private"
}

-- Poco dependencies
links { "PocoFoundation", "PocoNet" }

-- Conditionally include the testing project
if _OPTIONS["with-tests"] then
    -- HTTP Test Executable
    project "HTTPTest"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"
    objdir "bin/%{cfg.buildcfg}"

    files { "Source/Public/T_HTTPServer.cpp" }

    -- Link against the SDK and Poco
    links { "cplusplus-mcp-sdk", "PocoFoundation", "PocoNet" }

end

-- Compiler-specific options
filter "toolset:gcc or toolset:clang"
buildoptions { "-Wextra", "-Wpedantic" }

filter "toolset:msc"
buildoptions { "/W4", "/WX" }

-- Windows-specific libraries
filter "system:windows"
links { "ws2_32", "iphlpapi" }
systemversion "latest"
staticruntime "On"
cppdialect "C++20"