# Generates .vscode/cmake-kits.json for the current machine.
# Run once after cloning: .\tools\setup_kits.ps1

$ErrorActionPreference = "Stop"

# Find VsDevCmd.bat (first found under any VS version/edition)
$vsdev = (Get-ChildItem "${env:ProgramFiles(x86)}\Microsoft Visual Studio" `
    -Filter "VsDevCmd.bat" -Recurse -ErrorAction SilentlyContinue |
    Select-Object -First 1).FullName

if (-not $vsdev) {
    Write-Error "VsDevCmd.bat not found. Install Visual Studio with C++ workload."
    exit 1
}

Write-Host "Found: $vsdev"

# Capture environment after VsDevCmd.bat
$output = cmd /c "`"$vsdev`" -arch=x64 -host_arch=x64 >nul 2>&1 && set"
$env_vars = @{}
$output | Select-String '^(INCLUDE|LIB|LIBPATH)=(.+)$' | ForEach-Object {
    $env_vars[$_.Matches[0].Groups[1].Value] = $_.Matches[0].Groups[2].Value
}

if ($env_vars.Count -eq 0) {
    Write-Error "Failed to capture VS environment variables."
    exit 1
}

# Build a readable kit name from the path (e.g. "VS 2019 BuildTools")
$parts   = $vsdev -split '\\'
$year    = $parts | Where-Object { $_ -match '^\d{4}$' }   | Select-Object -First 1
$edition = $parts | Where-Object { $_ -match 'BuildTools|Community|Professional|Enterprise' } | Select-Object -First 1
$kitName = "Clang + VS $year $edition x64"

# Find clang++
$clangCmd = Get-Command clang++.exe -ErrorAction SilentlyContinue
$clang = if ($clangCmd) { $clangCmd.Source } else { "C:/Program Files/LLVM/bin/clang++.exe" }
$clangc = $clang -replace 'clang\+\+', 'clang'

$kit = @{
    name = $kitName
    compilers = @{ CXX = $clang; C = $clangc }
    preferredGenerator = @{ name = "Ninja" }
    environmentVariables = $env_vars
}

$json = ConvertTo-Json @($kit) -Depth 5
$out = Join-Path $PSScriptRoot "..\.vscode\cmake-kits-tmp.json"
Set-Content $out $json -Encoding utf8

Write-Host "Written: .vscode/cmake-kits-tmp.json" -ForegroundColor Green
Write-Host "Select kit in VS Code: Ctrl+Shift+P → CMake: Select a Kit" -ForegroundColor Cyan
