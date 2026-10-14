# VEX V5 Project - CLABKERDEPORTATION

This is a VEX V5 project that includes a DOOM port for the VEX V5 brain.

## Build Requirements

- VEXcode VS Code extension installed
- VEX V5 SDK installed (automatically installed with VEXcode)

## Building the Project

### Option 1: Using the provided build script (PowerShell)

```powershell
# Run from the project directory
.\build.ps1
```

### Option 2: Using the provided build script (Batch)

```cmd
build.bat
```

### Option 3: Manual build with make

First, ensure the toolchain is in your PATH:

```powershell
$env:PATH += ";C:/Users/kodie/AppData/Roaming/Code/User/globalStorage/vexrobotics.vexcode/tools/cpp/toolchain_win32/clang/bin;C:/Users/kodie/AppData/Roaming/Code/User/globalStorage/vexrobotics.vexcode/tools/cpp/toolchain_win32/gcc/bin;C:/Users/kodie/AppData/Roaming/Code/User/globalStorage/vexrobotics.vexcode/tools/cpp/toolchain_win32/tools/bin"
```

Then run make:

```powershell
make
```

For verbose output:

```powershell
make VERBOSE=1
```

To clean:

```powershell
make clean
```

## Build Output

The build produces:
- `build/CLABKERDEPORTATION.bin` - Binary file for VEX V5 brain
- `build/CLABKERDEPORTATION.elf` - ELF executable with debug symbols
- `build/CLABKERDEPORTATION.map` - Linker map file

## Project Structure

- `src/main.cpp` - Main application entry point
- `src/doom/` - DOOM port source files
- `include/vex.h` - VEX API headers
- `vex/mkenv.mk` - Build environment configuration
- `vex/mkrules.mk` - Build rules
- `makefile` - Main makefile

## Troubleshooting

### "math.h file not found" error

This error occurs when the toolchain PATH is not set correctly. The VEX toolchain uses a custom clang that doesn't include standard C headers in its own include directory. The headers are provided by the GCC toolchain in `gcc/include/`.

Make sure the PATH includes:
- `clang/bin` - for clang compiler
- `gcc/bin` - for linker and other tools
- `tools/bin` - for make

### "make not recognized" error

Make sure the VEX toolchain tools directory is in your PATH:
`C:/Users/kodie/AppData/Roaming/Code/User/globalStorage/vexrobotics.vexcode/tools/cpp/toolchain_win32/tools/bin`

## Notes

- The project uses a custom heap size of 12MB (0x00C00000) to accommodate the DOOM port
- The build targets ARM Cortex-A7 (armv7-a) with NEON support
- C++11 standard is used with RTTI and exceptions disabled for size optimization