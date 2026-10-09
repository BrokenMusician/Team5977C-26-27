# VEXcode mkenv.mk 2022_06_26_01

# macros to help with paths that include spaces
sp = $() $()
qs = $(subst ?, ,$1)
sq = $(subst $(sp),?,$1)

# default platform and build location
PLATFORM  = vexv5
BUILD     = build

# version for clang headers
ifneq ("$(origin HEADERS)", "command line")
HEADERS = 8.0.0
endif

# Project name passed from app
ifeq ("$(origin P)", "command line")
PROJECT  := $(P)
else
PROJECT  := $(call qs,$(notdir $(call sq,${CURDIR})))
endif

# check if the PROJECT name contains any whitespace
ifneq (1,$(words $(PROJECT)))
$(error Project name cannot contain whitespace: $(PROJECT))
endif

# SDK path passed from app
# if not set then environmental variabled used
ifeq ("$(origin T)", "command line")
VEX_SDK_PATH = $(T)/vexv5
endif
# backup if still not set
VEX_SDK_PATH ?= C:/Users/kodie/AppData/Roaming/Code/User/globalStorage/vexrobotics.vexcode/sdk/cpp/V5/V5_20240802_15_00_00/vexv5

# printf_float flag name passed from app (not used in this version)
ifeq ("$(origin PRINTF_FLOAT)", "command line")
PRINTF_FLAG = -u_printf_float
endif

# Verbose flag passed from app
ifeq ("$(origin V)", "command line")
BUILD_VERBOSE=$(V)
endif

# allow verbose to be set by makefile if not set by app
ifndef VERBOSE
BUILD_VERBOSE ?= 0
else
BUILD_VERBOSE ?= $(VERBOSE)
endif

# use verbose flag
ifeq ($(BUILD_VERBOSE),0)
Q = @
else
Q =
endif

# compile and link tools
# Toolchain path - try to find it relative to SDK or use environment
ifeq ("$(origin TOOLCHAIN_PATH)", "command line")
# Use provided toolchain path
else
# Default to standard VEX extension location
TOOLCHAIN_PATH = C:/Users/kodie/AppData/Roaming/Code/User/globalStorage/vexrobotics.vexcode/tools/cpp/toolchain_win32
endif

CC      = "$(TOOLCHAIN_PATH)/clang/bin/clang.exe"
CXX     = "$(TOOLCHAIN_PATH)/clang/bin/clang.exe"
OBJCOPY = "$(TOOLCHAIN_PATH)/gcc/bin/arm-none-eabi-objcopy.exe"
SIZE    = "$(TOOLCHAIN_PATH)/gcc/bin/arm-none-eabi-size.exe"
LINK    = "$(TOOLCHAIN_PATH)/gcc/bin/arm-none-eabi-ld.exe"
ARCH    = "$(TOOLCHAIN_PATH)/gcc/bin/arm-none-eabi-ar.exe"
ECHO    = @echo
DEFINES = -DVexV5

# platform specific macros
ifeq ($(OS),Windows_NT)
$(info windows build for platform $(PLATFORM))
SHELL = cmd.exe
MKDIR = md "$(@D)" 2> nul || :
RMDIR = rmdir /S /Q
CLEAN = $(RMDIR) $(BUILD) 2> nul || :
else
# which flavor of linux
UNAME := $(shell sh -c 'uname -sm 2>/dev/null || Unknown')
$(info unix build for platform $(PLATFORM) on $(UNAME))
MKDIR = mkdir -p "$(@D)" 2> /dev/null || :
RMDIR = rm -rf
CLEAN = $(RMDIR) $(BUILD) 2> /dev/null || :
endif

# toolchain include and lib locations
TOOL_INC  = -I"$(VEX_SDK_PATH)/clang/$(HEADERS)/include" -I"$(VEX_SDK_PATH)/gcc/include/c++/4.9.3"  -I"$(VEX_SDK_PATH)/gcc/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb" -I"$(VEX_SDK_PATH)/gcc/include"
TOOL_LIB  = -L"$(VEX_SDK_PATH)/gcc/libs"

# compiler flags
CFLAGS_CL = -target thumbv7-none-eabi -fshort-enums -Wno-unknown-attributes -U__INT32_TYPE__ -U__UINT32_TYPE__ -D__INT32_TYPE__=long -D__UINT32_TYPE__='unsigned long' 
CFLAGS_V7 = -march=armv7-a -mfpu=neon -mfloat-abi=softfp
CFLAGS    = ${CFLAGS_CL} ${CFLAGS_V7} -Os -Wall -Werror=return-type -ansi -std=gnu99 $(DEFINES)
CXX_FLAGS = ${CFLAGS_CL} ${CFLAGS_V7} -Os -Wall -Werror=return-type -fno-rtti -fno-threadsafe-statics -fno-exceptions  -std=gnu++11 -ffunction-sections -fdata-sections $(DEFINES)

# linker flags
# Doom needs a multi-megabyte zone allocator; VEX's linker script defaults
# the C heap to 1 MiB, which is too small for the game.
LNK_FLAGS = --defsym=_HEAP_SIZE=0x00C00000 -nostdlib -T "$(VEX_SDK_PATH)/lscript.ld" -R "$(VEX_SDK_PATH)/stdlib_0.lib" -Map="$(BUILD)/$(PROJECT).map" --gc-section -L"$(VEX_SDK_PATH)" ${TOOL_LIB}

# future statuc library
PROJECTLIB = lib$(PROJECT)
ARCH_FLAGS = rcs

# libraries
LIBS =  --start-group -lv5rt -lstdc++ -lc -lm -lgcc --end-group

# include file paths
INC += $(addprefix -I, ${INC_F})
INC += -I"$(VEX_SDK_PATH)/include"
INC += ${TOOL_INC}
