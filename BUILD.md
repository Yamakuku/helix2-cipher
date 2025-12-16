# Building Helix2-cipher

This document provides detailed instructions for building the Helix2 stream cipher on various platforms.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Windows with MinGW](#windows-with-mingw)
- [Linux](#linux)
- [Build Targets](#build-targets)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### All Platforms

- A C compiler supporting C11 standard
- Make utility
- Basic development tools

### Platform-Specific Requirements

#### Windows
- **MinGW-w64** or **MSYS2** with GCC
- Git Bash or MSYS2 shell (for make)

#### Linux
- GCC (recommended) or Clang
- Make (usually pre-installed)
- Standard development tools


## Windows with MinGW (MSYS2)

### Debug build
``` bash
   make
```

### Release build with optimizations
``` bash
   make release
```

### Run tests
build\debug\helix2_test

## Linux

### Debug build
``` bash
   make
```

### Release build with optimizations
``` bash
   make release
```

### Run tests
``` bash
   ./build/debug/helix2_test
```

## Build Targets

### Primary Targets

| Command | Description |
|---------|-------------|
| `make` | Default debug build |
| `make debug` | Explicit debug build |
| `make release` | Optimized release build |
| `make all` | Same as `make` |
| `make clean` | Remove all build artifacts |
| `make clean-debug` | Remove debug artifacts only |
| `make clean-release` | Remove release artifacts only |

### Debug Build Output

Located in `build/debug/`:
- `libhelix2.a` - Static library
- `helix2_cl.exe` - Command-line tool
- `helix2_test.exe` - Test suite
- `obj/` - Object files

Debug flags: `-g -O0 -Wall`

### Release Build Output

Located in `build/release/`:
- `libhelix2.a` - Optimized static library
- `helix2_cl.exe` - Optimized command-line tool
- `helix2_performance.exe` - Benchmark utility
- `obj/` - Object files

Release flags: `-O3 -march=native -ffast-math -funroll-loops -DNDEBUG -Wall`

## Using the Static Library

### Compile your program
gcc -c myprogram.c -I/path/to/helix2-cipher/src

### Link with the library
gcc myprogram.o -L/path/to/helix2-cipher/build/release -llibhelix2.a -o myprogram

## Test the CLI Tool

### Create test file
echo "Hello, Helix2!" > test.txt

### Encrypt
./build/release/helix2_cl -e -p "test123" -n 00000000000000000000000000000000000000000000 test.txt -o test.enc

### Decrypt
./build/release/helix2_cl -d -p "test123" -n 00000000000000000000000000000000000000000000 test.enc -o test.dec

### Verify
diff test.txt test.dec


## Getting Help

If you encounter build issues:

1. Check the [README.md](README.md) for general information
2. Verify prerequisites are installed
3. Check compiler and make versions
4. Review error messages carefully
5. Open an issue on GitHub with:
   - Operating system and version
   - Compiler version (`gcc --version`)
   - Make version (`make --version`)
   - Full error output
   - Steps to reproduce

## See Also

- [README.md](README.md) - Project overview
- [LICENSE](LICENSE) - License information
