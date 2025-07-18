@echo off
setlocal

:: Define the path to the premake executable and your workspace name
set PREMAKE_EXE=vendor\bin\premake\premake5.exe
set WORKSPACE_NAME=cplusplus-mcp-sdk

echo Checking for existing files to clean...

:: -----------------------------------------------------
:: 1. Clean up old files and directories if they exist
:: -----------------------------------------------------

:: Remove the main solution file
if exist "%WORKSPACE_NAME%.sln" (
    echo  - Removing old solution file...
    del /q "%WORKSPACE_NAME%.sln"
)

:: Remove all Visual Studio project files
if exist "*.vcxproj" (
    echo  - Removing old project files...
    del /q "*.vcxproj"
    del /q "*.vcxproj.filters"
    del /q "*.vcxproj.user"
)

:: Remove the Makefile
if exist "Makefile" (
    echo  - Removing old Makefile...
    del /q "Makefile"
)

:: Remove output and cache directories
if exist "bin" (
    echo  - Removing bin directory...
    rd /s /q "bin"
)
if exist "bin-obj" (
    echo  - Removing obj directory...
    rd /s /q "bin-obj"
)

echo.
echo Cleaning complete.
echo ----------------------------------------
echo.

:: -----------------------------------------------------
:: 2. Generate new project files
:: -----------------------------------------------------
echo Generating new project files...
call %PREMAKE_EXE% gmake
call %PREMAKE_EXE% vs2017

echo.
echo Done