@echo off
REM AgentLog Build Script for Windows
REM Supports MSVC and MinGW

setlocal enabledelayedexpansion

REM Default values
set BUILD_TYPE=Release
set BUILD_DIR=build
set GENERATOR=
set BUILD_TESTS=ON
set BUILD_EXAMPLES=ON
set JOBS=%NUMBER_OF_PROCESSORS%

REM Parse command line arguments
:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="--help" goto usage
if /i "%~1"=="-h" goto usage
if /i "%~1"=="--type" (
    set BUILD_TYPE=%~2
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--dir" (
    set BUILD_DIR=%~2
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--jobs" (
    set JOBS=%~2
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--vs2019" (
    set GENERATOR="Visual Studio 16 2019"
    shift
    goto parse_args
)
if /i "%~1"=="--vs2022" (
    set GENERATOR="Visual Studio 17 2022"
    shift
    goto parse_args
)
if /i "%~1"=="--mingw" (
    set GENERATOR="MinGW Makefiles"
    shift
    goto parse_args
)
if /i "%~1"=="--no-tests" (
    set BUILD_TESTS=OFF
    shift
    goto parse_args
)
if /i "%~1"=="--no-examples" (
    set BUILD_EXAMPLES=OFF
    shift
    goto parse_args
)
if /i "%~1"=="--clean" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    shift
    goto parse_args
)
shift
goto parse_args

:end_parse

echo ================================================
echo AgentLog Windows Build Script
echo ================================================
echo Build type: %BUILD_TYPE%
echo Build directory: %BUILD_DIR%
echo Parallel jobs: %JOBS%
if defined GENERATOR echo Generator: %GENERATOR%
echo ================================================
echo.

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Check for vcpkg (recommended for Windows dependencies)
if defined VCPKG_ROOT (
    echo Found vcpkg at: %VCPKG_ROOT%
    set CMAKE_TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
) else (
    echo Warning: VCPKG_ROOT not set. You may need to manually specify dependency locations.
    set CMAKE_TOOLCHAIN=
)

REM Run CMake configuration
echo Running CMake configuration...
if defined GENERATOR (
    cmake -G %GENERATOR% ^
          -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
          -DAGENTLOG_BUILD_TESTS=%BUILD_TESTS% ^
          -DAGENTLOG_BUILD_EXAMPLES=%BUILD_EXAMPLES% ^
          %CMAKE_TOOLCHAIN% ^
          ..
) else (
    cmake -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
          -DAGENTLOG_BUILD_TESTS=%BUILD_TESTS% ^
          -DAGENTLOG_BUILD_EXAMPLES=%BUILD_EXAMPLES% ^
          %CMAKE_TOOLCHAIN% ^
          ..
)

if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo.
echo Building with %JOBS% parallel jobs...
cmake --build . --config %BUILD_TYPE% --parallel %JOBS%

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

REM Run tests if enabled
if "%BUILD_TESTS%"=="ON" (
    echo.
    echo Running tests...
    ctest --output-on-failure --build-config %BUILD_TYPE%
)

echo.
echo ================================================
echo Build completed successfully!
echo ================================================
echo.
echo To install, run:
echo   cd %BUILD_DIR% ^&^& cmake --install . --config %BUILD_TYPE%
echo.
echo To run examples:
echo   cd %BUILD_DIR%\bin\%BUILD_TYPE%
echo   basic_usage.exe
echo.

goto end

:usage
echo Usage: build.bat [OPTIONS]
echo.
echo Build AgentLog library for Windows
echo.
echo OPTIONS:
echo     --help              Show this help message
echo     --type TYPE         Build type: Debug, Release, RelWithDebInfo (default: Release)
echo     --dir DIR           Build directory (default: build)
echo     --jobs N            Number of parallel jobs (default: %NUMBER_OF_PROCESSORS%)
echo     --vs2019            Use Visual Studio 2019 generator
echo     --vs2022            Use Visual Studio 2022 generator
echo     --mingw             Use MinGW Makefiles generator
echo     --no-tests          Disable building tests
echo     --no-examples       Disable building examples
echo     --clean             Clean build directory before building
echo.
echo EXAMPLES:
echo     # Build with Visual Studio 2022
echo     build.bat --vs2022
echo.
echo     # Debug build with MinGW
echo     build.bat --mingw --type Debug
echo.
echo     # Clean build for distribution
echo     build.bat --clean --type Release --no-tests
echo.
goto end

:end
endlocal
