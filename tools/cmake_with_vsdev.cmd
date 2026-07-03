@echo off

:: Use vswhere.exe to find the latest Visual Studio installation (2017+).
:: Works with VS 2019, 2022, Community, Professional, Enterprise, BuildTools.
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq tokens=*" %%i in (
    `%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`
) do set VS_PATH=%%i

if not defined VS_PATH (
    echo ERROR: No Visual Studio installation with C++ tools found.
    echo Install "Desktop development with C++" workload via Visual Studio Installer.
    exit /b 1
)

call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

:: Check that CMake supports --preset (requires 3.19+, project needs 3.24+)
for /f "tokens=3" %%v in ('cmake --version 2^>^&1 ^| findstr /i "cmake version"') do set CMAKE_VER=%%v
for /f "tokens=1,2 delims=." %%a in ("%CMAKE_VER%") do (
    if %%a LSS 3 (
        echo ERROR: CMake %%a.%%b found, but version 3.24+ is required.
        echo Run: winget install Kitware.CMake
        exit /b 1
    )
    if %%a EQU 3 if %%b LSS 24 (
        echo ERROR: CMake %CMAKE_VER% found, but version 3.24+ is required.
        echo Run: winget install Kitware.CMake
        exit /b 1
    )
)

cmake %*
exit /b %errorlevel%
