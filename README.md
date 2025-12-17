# Helix2 Stream Cipher

So one day I thought

`I wonder how you build cryptographic ciphers`

Discovering the stream cipher design, where it is possible to process down to a single byte, anywhere in the buffer, was an eye opener.

And sometimes later Helix2 was born. An educational ARX (Add-Rotate-XOR) stream cipher (like ChaCha20), but with some fundemental differences, designed for learning and experimentation with cryptographic primitives.

Though the process was as much more on how to test your cipher, going back and forth, untill you have a result, that would justify all the time you spend on this project.

## ⚠️ Security Warning

**This cipher is experimental and has NOT undergone formal cryptanalysis. It should NOT be used for production security applications.** 

## Overview

Helix2 is a stream cipher that explores nested ARX operations for higher per-operation complexity compared to traditional designs. It was created as an educational project to understand stream cipher design principles.

### Design Specifications

- **Key Size**: 256-bit (32 bytes)
- **Nonce Size**: 160-bit (20 bytes)
- **Block Size**: 64 bytes
- **Counter**: 64-bit block counter supporting massive data volumes
- **Rounds**: each round with intermediate state addition
- **Pattern**: 8 shuffles per round, 1st round ; Row-wise mixing (horizontal) and Diagonal mixing (cross-diffusion), 2nd round ; Column-wise mixing (vertical) and mirrored Diagonal mixing (cross-diffusion)
- **Operations**: 8 operations per shuffle (4 compound + 4 simple)

### Statistical Testing Results

The cipher has been tested with industry-standard statistical test suites:

- **Dieharder**: All tests passed ✓
- **PractRand**: Passed 32 GB with no anomalies ✓

## Performance Benchmarks

Performance comparison on the same hardware:

| Cipher      | Average Throughput |
|-------------|--------------------|
| ChaCha20    | ~575 MB/s          |
| Helix2      | ~850 MB/s          |

*Note: Benchmarks performed on a single core. Performance may vary by platform.*

## Building

### Prerequisites

- **Windows**: MinGW-w64 with GCC
- **Linux**: GCC or Clang

### Quick Start

#### Debug Build
```bash
make
```

This builds:
- `libhelix2.a` - Static library
- `helix2_cl.exe` - Command-line tool
- `helix2_test.exe` - Test suite

#### Release Build
```bash
make release
```

This builds optimized versions plus `helix2_performance.exe` for benchmarking, but excluding the Test suite.

#### Clean Build
```bash
make clean        # Clean all builds
make clean-debug  # Clean debug only
make clean-release # Clean release only
```

For detailed build instructions, see [BUILD.md](BUILD.md).

## Usage

### Command-Line Tool

Encrypt a file:
```bash
helix2_cl -e -p "your_password" -n 0123456789abcdef0123456789abcdef01234567 input.txt -o encrypted.bin
```

Decrypt a file:
```bash
helix2_cl -d -p "your_password" -n 0123456789abcdef0123456789abcdef01234567 encrypted.bin -o decrypted.txt
```

Options:
- `-e` : Encrypt mode
- `-d` : Decrypt mode
- `-p <password>` : Encryption password (required)
- `-n <nonce>` : 160-bit nonce as 40 hex characters (20 bytes)
- `-o <output>` : Output filename (optional, overwrites input if omitted)

### Library API

```c
#include "helix2.h"

// Initialize context
helix2_context_t ctx;
uint8_t key[32] = { /* your key */ };
uint8_t nonce[20] = { /* your nonce */ };
helix2_initialize_context(&ctx, key, nonce);

// Encrypt/decrypt a single byte
uint8_t encrypted = helix2_byte(&ctx, offset, plaintext_byte);

// Encrypt/decrypt a buffer
uint8_t buffer[1024] = { /* your data */ };
helix2_buffer(&ctx, buffer, sizeof(buffer), start_offset);

// Seek to a specific block
helix2_buffer_set_next_block(&ctx, block_index);
```

## Testing

Run the test suite:
```bash
./build/debug/helix2_test.exe
```

Tests include:
- Symmetry (encrypt/decrypt produces original)
- Determinism (same key/nonce produces same output)
- Offset seeking (random access works correctly)
- Entropy (output appears uniformly distributed)
- Test vectors (known answer tests)
- 64-bit counter functionality

Run performance benchmarks:
```bash
./build/release/helix2_performance.exe
```

## Security Considerations

1. **Not for Production**: This is an experimental cipher for educational purposes
2. **Nonce Reuse**: Never reuse a nonce with the same key
3. **Key Management**: Use proper key derivation functions (e.g., Argon2, scrypt)
4. **Authentication**: This cipher provides no authentication
5. **Side Channels**: No protection against timing or cache attacks

## Technical Details

### Keystream Generation

The cipher uses a 16-word (64-byte) state:
- Words 0-1: Magic constant `"so!M4g1c"`
- Words 2-9: 256-bit key
- Word 10: Block counter (low 32 bits)
- Word 11: Nonce[0:4] XOR counter high bits
- Words 12-15: Nonce[4:20]

### Round Function

Each round performs:
1. Column mixing (4 shuffle operations)
2. Row mixing (4 shuffle operations)
3. Diagonal mixing (4 shuffle operations)
4. State addition

The shuffle operation combines compound and simple ARX operations with varying rotation constants.

For more information, goto https://github.com/Yamakuku/helix2-cipher/wiki

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

**Jarl "Yamakuku" Lindeneg**
- First GitHub project upload (be kind)

## Acknowledgments

- Inspired by [ChaCha20](https://datatracker.ietf.org/doc/html/rfc8439) by Daniel J. Bernstein
- Statistical testing using [Dieharder](https://webhome.phy.duke.edu/~rgb/General/dieharder.php) and [PractRand](https://pracrand.sourceforge.net/)
- Educational purposes only

## Version History

- **v2.1** (December 8, 2025)
  - Current release
  - 64-bit block counter support
  - Comprehensive test suite
  - Command-line tool
  - Performance benchmarks

## FAQ

**Q: Why create a new cipher?**
A: Purely educational. Understanding how ciphers work by building one is a valuable learning experience.

**Q: Can I use this in my project?**
A: Yes (MIT license), but please don't use it for anything requiring proven security.
