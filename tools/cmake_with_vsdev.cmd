@echo off

:: Use vswhere.exe to find the latest Visual Studio installation (2017+).
:: Works with VS 2019, 2022, Community, Professional, Enterprise, BuildTools.
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq tokens=*" %%i in (
    `%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`
) do set VS_PATH=%%i


if not defined VS_PATH (
    echo ERROR: No Visual Studio installation with C++ tools found.
    echo Install "MSVC v14x compiler" component via Visual Studio Installer.
    exit /b 1
)


call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

cmake %*
exit /b %errorlevel%
