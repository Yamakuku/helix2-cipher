# Helix2 Stream Cipher - Build Configuration
# Copyright (c) 2025 Jarl "Yamakuku" Lindeneg
# License: MIT

# Compiler and directories
CC := gcc
AR := ar
SRCDIR := src
INCDIR := $(SRCDIR)

# Platform detection (Windows and Linux)
ifeq ($(OS),Windows_NT)
    # Windows (MSYS2/MinGW/Cygwin)
    PLATFORM := Windows
    EXE_EXT := .exe
    PLATFORM_CFLAGS := -mconsole
else
    # Linux (other Unix-like systems may work but are untested)
    PLATFORM := Linux
    EXE_EXT :=
    PLATFORM_CFLAGS :=
endif

# Compiler flags with platform-specific options
CFLAGS_DEBUG := -g -O0 -Wall -std=c11 $(PLATFORM_CFLAGS) -I$(INCDIR)
CFLAGS_RELEASE := -O3 -march=native -ffast-math -funroll-loops -DNDEBUG -Wall -std=c11 $(PLATFORM_CFLAGS) -I$(INCDIR)

# Source files
SRC_HELIX2     := $(SRCDIR)/helix2.c
SRC_HELIX2_TEST := $(SRCDIR)/../tests/helix2_test.c
SRC_HELIX2_PERF := $(SRCDIR)/../tests/helix2_performance.c

SRC_HELIX2_CL     := $(SRCDIR)/../tools/helix2_cl.c

# Library names
LIB_HELIX2 := libhelix2.a

.PHONY: all debug release clean clean-debug clean-release dirs-debug dirs-release

all: debug

# ============================================================================
# DEBUG BUILD
# ============================================================================
# Builds: libraries + cl + test executable (NO performance)
debug: dirs-debug \
		build/debug/$(LIB_HELIX2) \
		build/debug/helix2_cl$(EXE_EXT) \
		build/debug/helix2_test$(EXE_EXT)
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║          Debug build complete in build/debug/                  ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║  Libraries:                                                    ║"
	@echo "║    • libhelix2.a                                               ║"
	@echo "║  Executables:                                                  ║"
	@echo "║    • helix2_cl.exe      (command-line tool)                    ║"
	@echo "║    • helix2_test.exe    (Helix2 unit tests)                    ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"

# ============================================================================
# RELEASE BUILD
# ============================================================================
# Builds: libraries + cl + performance executable (NO tests)
release: dirs-release \
		build/release/$(LIB_HELIX2) \
		build/release/helix2_cl$(EXE_EXT) \
		build/release/helix2_performance$(EXE_EXT)
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║       Release build complete in build/release/                 ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║  Libraries:                                                    ║"
	@echo "║    • libhelix2.a                                               ║"
	@echo "║  Executables:                                                  ║"
	@echo "║    • helix2_cl.exe              (command-line tool)            ║"
	@echo "║    • helix2_performance.exe     (Helix2 benchmark)             ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"

# ============================================================================
# DIRECTORY CREATION
# ============================================================================
dirs-debug:
	@mkdir -p build/debug/obj

dirs-release:
	@mkdir -p build/release/obj

# ============================================================================
# DEBUG BUILD RULES
# ============================================================================

# Helix2 object - DEBUG
build/debug/obj/helix2.o: $(SRC_HELIX2)
	$(CC) -c $(CFLAGS_DEBUG) -o "$@" "$<"

build/debug/obj/helix2_test.o: $(SRC_HELIX2_TEST)
	$(CC) -c $(CFLAGS_DEBUG) -o "$@" "$<"	

# helix2_cl object - DEBUG
build/debug/obj/helix2_cl.o: $(SRC_HELIX2_CL)
	$(CC) -c $(CFLAGS_DEBUG) -o "$@" "$<"

# Libraries - DEBUG
build/debug/$(LIB_HELIX2): build/debug/obj/helix2.o	
	$(AR) rcs "$@" "$<"
	
# Test executables - DEBUG
build/debug/helix2_test$(EXE_EXT): build/debug/obj/helix2_test.o build/debug/$(LIB_HELIX2)
	$(CC) $(CFLAGS_DEBUG) -o "$@" build/debug/obj/helix2_test.o build/debug/$(LIB_HELIX2)	

# Command-line tool - DEBUG (uses both libraries)
build/debug/helix2_cl$(EXE_EXT): build/debug/obj/helix2_cl.o build/debug/$(LIB_HELIX2)
	$(CC) $(CFLAGS_DEBUG) -o "$@" build/debug/obj/helix2_cl.o build/debug/$(LIB_HELIX2)
# ============================================================================
# RELEASE BUILD RULES
# ============================================================================

# Helix2 objects - RELEASE
build/release/obj/helix2.o: $(SRC_HELIX2)
	$(CC) -c $(CFLAGS_RELEASE) -o "$@" "$<"

build/release/obj/helix2_performance.o: $(SRC_HELIX2_PERF)
	$(CC) -c $(CFLAGS_RELEASE) -o "$@" "$<"	

# helix2_cl object - RELEASE
build/release/obj/helix2_cl.o: $(SRC_HELIX2_CL)
	$(CC) -c $(CFLAGS_RELEASE) -o "$@" "$<"

build/release/$(LIB_HELIX2): build/release/obj/helix2.o
	$(AR) rcs "$@" "$<"	

# Performance executable - RELEASE
build/release/helix2_performance$(EXE_EXT): build/release/obj/helix2_performance.o build/release/$(LIB_HELIX2)
	$(CC) $(CFLAGS_RELEASE) -o "$@" build/release/obj/helix2_performance.o build/release/$(LIB_HELIX2)

# Command-line tool - RELEASE
build/release/helix2_cl$(EXE_EXT): build/release/obj/helix2_cl.o build/release/$(LIB_HELIX2)
	$(CC) $(CFLAGS_RELEASE) -o "$@" build/release/obj/helix2_cl.o build/release/$(LIB_HELIX2)
# ============================================================================
# CLEAN TARGETS
# ============================================================================
clean-debug:
	-rm -rf build/debug
	@echo "Debug build cleaned."

clean-release:
	-rm -rf build/release
	@echo "Release build cleaned."

clean: clean-debug clean-release
	-rm -rf build
	@echo "All builds cleaned."