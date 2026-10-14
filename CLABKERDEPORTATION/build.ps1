#!/usr/bin/env pwsh
# Build script for VEX V5 project
# This script sets up the PATH and runs make

$toolchainPath = "C:/Users/kodie/AppData/Roaming/Code/User/globalStorage/vexrobotics.vexcode/tools/cpp/toolchain_win32"
$env:PATH += ";$toolchainPath/clang/bin;$toolchainPath/gcc/bin;$toolchainPath/tools/bin"

cd "c:\Users\Kodie\Vez\Team5977C-26-27\CLABKERDEPORTATION"

# Parse arguments
$verbose = $false
$clean = $false

foreach ($arg in $args) {
    if ($arg -eq "-v" -or $arg -eq "--verbose") {
        $verbose = $true
    }
    elseif ($arg -eq "-c" -or $arg -eq "--clean") {
        $clean = $true
    }
}

if ($clean) {
    Write-Host "Cleaning build directory..."
    make clean
}

$makeArgs = @()
if ($verbose) {
    $makeArgs += "VERBOSE=1"
}

Write-Host "Building project..."
make @makeArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful!" -ForegroundColor Green
    Write-Host "Output: build/CLABKERDEPORTATION.bin"
} else {
    Write-Host "Build failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}