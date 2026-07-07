@echo off
:: Build the project for WebAssembly using Emscripten.
::
:: Usage:
::   .\tools\build_wasm.cmd [preset] [emsdk_path]
::
::   preset     — wasm-release (default) or wasm-debug
::   emsdk_path — path to emsdk root, e.g. C:\var\wasm\emsdk
::                (optional if EMSDK env var is already set)
::
:: From PowerShell, env vars set in .bat don't propagate back.
:: Pass the path as argument or set it first:
::   $env:EMSDK = "C:\var\wasm\emsdk"
::   .\tools\build_wasm.cmd

set PRESET=wasm-release
if not "%1"=="" if not "%1"=="wasm-debug" if not "%1"=="wasm-release" (
    :: First arg looks like a path, not a preset name
    if not defined EMSDK set EMSDK=%1
    goto :find_preset
)
if not "%1"=="" set PRESET=%1
if not "%2"=="" if not defined EMSDK set EMSDK=%2

:find_preset
if not defined EMSDK (
    echo ERROR: EMSDK not set.
    echo.
    echo Option 1 — pass emsdk path as argument:
    echo   .\tools\build_wasm.cmd wasm-release C:\var\wasm\emsdk
    echo.
    echo Option 2 — set in PowerShell before calling:
    echo   $env:EMSDK = "C:\var\wasm\emsdk"
    echo   .\tools\build_wasm.cmd
    exit /b 1
)

call "%EMSDK%\emsdk_env.bat" >nul 2>&1

cmake --preset %PRESET%
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset %PRESET%
exit /b %errorlevel%
