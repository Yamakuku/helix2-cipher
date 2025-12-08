# Changelog

All notable changes to the Helix-2 Stream Cipher project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.0] - 2025-12-08

### Added
- 64-bit block counter support for processing massive data volumes (up to 2^64 blocks)
- Test suite with 6 test categories
- Performance benchmark utility
- Command-line encryption/decryption tool with password-based key derivation
- Statistical test results (Dieharder and PractRand)
- GitHub documentation (README, BUILD, CONTRIBUTING, LICENSE)
- Makefile build system for Windows (MinGW), Linux, and macOS

### Changed
- Improved nonce handling for better security
- Enhanced test coverage including 64-bit counter tests
- Optimized keystream generation
- Updated documentation with security warnings

### Fixed
- Endianness handling for cross-platform compatibility
- Buffer boundary conditions
- Memory initialization issues

### Security
- Added explicit security disclaimers
- Documented that this is an experimental cipher for educational use only
- Not recommended for production use

## [2.0.0] - 2025-12-XX

### Added
- Initial implementation of Helix-2 stream cipher
- ARX (Add-Rotate-XOR) based design
- 2-round structure with state addition
- 256-bit key and 160-bit nonce support
- Basic test vectors

### Design Goals
- Educational exploration of stream cipher design
- Higher per-operation complexity than ChaCha20
- Nested ARX operations for complexity
- Good statistical properties


## Version Numbering

- **Major version**: Breaking API changes or significant algorithm changes
- **Minor version**: New features, backwards compatible
- **Patch version**: Bug fixes, documentation updates

## Links

- [Repository](https://github.com/YOUR_USERNAME/helix2-cipher)
- [Issue Tracker](https://github.com/YOUR_USERNAME/helix2-cipher/issues)
- [Releases](https://github.com/YOUR_USERNAME/helix2-cipher/releases)

## Notes

This project is actively developed for educational purposes. Contributions are welcome!
