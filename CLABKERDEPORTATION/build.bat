@echo off
REM Build script for VEX V5 project
REM This script sets up the PATH and runs make

set TOOLCHAIN_PATH=C:\Users\kodie\AppData\Roaming\Code\User\globalStorage\vexrobotics.vexcode\tools\cpp\toolchain_win32
set PATH=%TOOLCHAIN_PATH%\clang\bin;%TOOLCHAIN_PATH%\gcc\bin;%TOOLCHAIN_PATH%\tools\bin;%PATH%

cd /d "c:\Users\Kodie\Vez\Team5977C-26-27\CLABKERDEPORTATION"

set VERBOSE=0
set CLEAN=0

:parse_args
if "%~1"=="" goto :build
if "%~1"=="-v" set VERBOSE=1
if "%~1"=="--verbose" set VERBOSE=1
if "%~1"=="-c" set CLEAN=1
if "%~1"=="--clean" set CLEAN=1
shift
goto :parse_args

:build
if %CLEAN%==1 (
    echo Cleaning build directory...
    make clean
)

if %VERBOSE%==1 (
    echo Building project (verbose)...
    make VERBOSE=1
) else (
    echo Building project...
    make
)

if %ERRORLEVEL%==0 (
    echo Build successful!
    echo Output: build\CLABKERDEPORTATION.bin
) else (
    echo Build failed!
    exit /b %ERRORLEVEL%
)