# BitChat Scripts Directory

This directory contains all the organized build, setup, and test scripts for BitChat across different platforms.

## 🚀 Quick Start

Use the main script launcher for all operations:

```bash
# Setup development environment (auto-detects platform)
./scripts/bitchat-setup.sh setup

# Build for your platform
./scripts/bitchat-setup.sh build

# Run tests
./scripts/bitchat-setup.sh test

# Install system-wide
./scripts/bitchat-setup.sh install

# Clean build artifacts
./scripts/bitchat-setup.sh clean

# Check project status
./scripts/bitchat-setup.sh status
```

## 📁 Directory Structure

```
scripts/
├── bitchat-setup.sh           # Main script launcher (USE THIS!)
├── setup/                     # Platform setup scripts
│   └── ios-setup.sh          # iOS/macOS development setup
├── build/                     # Build scripts
│   └── linux-build.sh        # Linux build script (comprehensive)
├── test/                      # Test scripts
│   └── linux-test.sh         # Linux test suite
├── nixos/                     # NixOS-specific scripts
│   ├── nixos-install.sh       # NixOS installation script
│   └── nixos-test.sh          # NixOS test suite
└── README.md                  # This file
```

## 🎯 Platform-Specific Usage

### iOS/macOS
```bash
# Setup Xcode project
./scripts/bitchat-setup.sh setup --ios

# Build instructions (use Xcode)
./scripts/bitchat-setup.sh build --ios
```

### Linux (Standard Distributions)
```bash
# Build with automatic dependency installation
./scripts/build/linux-build.sh --install-deps

# Build in debug mode with verbose output
./scripts/build/linux-build.sh --debug --verbose

# Build and install system-wide
./scripts/build/linux-build.sh --install

# Run comprehensive tests
./scripts/test/linux-test.sh
```

### NixOS
```bash
# Install system-wide service
./scripts/nixos/nixos-install.sh

# Run NixOS-specific tests
./scripts/nixos/nixos-test.sh

# Or use the main launcher
./scripts/bitchat-setup.sh setup --nixos
./scripts/bitchat-setup.sh test --nixos
```

## 🔧 Script Features

### Main Launcher (`bitchat-setup.sh`)
- **Auto-detection**: Automatically detects your platform (macOS, Linux, NixOS)
- **Unified interface**: Single command for all operations
- **Cross-platform**: Works on all supported platforms
- **Status checking**: Shows project status and available scripts

### Linux Build Script (`build/linux-build.sh`)
- **Dependency management**: Automatic installation for major distributions
- **Multi-distribution support**: Ubuntu, Fedora, Arch, openSUSE
- **Build options**: Debug/Release, verbose output, clean builds
- **System integration**: Desktop file and icon installation
- **Bluetooth setup**: Automatic capability configuration
- **Testing**: Built-in build verification

### Linux Test Script (`test/linux-test.sh`)
- **Comprehensive testing**: 10 different test categories
- **Dependency checking**: Runtime and build dependencies
- **Bluetooth testing**: Service and adapter verification
- **Configuration testing**: Config file creation and validation
- **Performance testing**: Binary size and security features
- **System integration**: Desktop file and icon verification

### NixOS Scripts
- **Multiple installation methods**: System service, user install, development
- **NixOS integration**: Proper module and service setup
- **Nix-specific testing**: Build environment, flakes, garbage collection
- **Configuration management**: Declarative NixOS configuration

## 🛠️ Advanced Usage

### Linux Build Options
```bash
# Full build with all options
./scripts/build/linux-build.sh \
  --install-deps \
  --clean \
  --verbose \
  --install

# Debug build for development
./scripts/build/linux-build.sh \
  --debug \
  --verbose \
  --clean
```

### Testing Options
```bash
# Run tests with different configurations
./scripts/test/linux-test.sh

# NixOS-specific comprehensive tests
./scripts/nixos/nixos-test.sh
```

## 📋 Requirements

### Linux
- CMake 3.16+
- Qt6 (Base, Widgets, Bluetooth)
- libsodium
- zlib
- GLib 2.0
- D-Bus
- libcap

### NixOS
- Nix with flakes (optional)
- Bluetooth enabled in configuration
- User in bluetooth group

### macOS
- Xcode 12+
- XcodeGen (optional)
- Swift Package Manager

## 🔍 Troubleshooting

### Common Issues

1. **Missing dependencies**
   ```bash
   # Linux: Use --install-deps flag
   ./scripts/build/linux-build.sh --install-deps
   ```

2. **Bluetooth permissions**
   ```bash
   # Linux: Capabilities are set automatically
   sudo setcap cap_net_raw+ep ./bitchat-linux/build/bitchat-linux
   ```

3. **NixOS Bluetooth not working**
   ```nix
   # Add to configuration.nix
   hardware.bluetooth.enable = true;
   users.users.yourusername.extraGroups = [ "bluetooth" ];
   ```

### Getting Help

1. **Check project status**
   ```bash
   ./scripts/bitchat-setup.sh status
   ```

2. **Run platform-specific tests**
   ```bash
   ./scripts/bitchat-setup.sh test
   ```

3. **View script help**
   ```bash
   ./scripts/bitchat-setup.sh help
   ./scripts/build/linux-build.sh --help
   ```

## 🎉 Migration from Old Scripts

The old scripts in the main directory have been replaced:

| Old Script | New Script |
|------------|------------|
| `setup.sh` | `scripts/setup/ios-setup.sh` |
| `build.sh` | `scripts/build/linux-build.sh` |
| `test_bitchat.sh` | `scripts/test/linux-test.sh` |
| `install-nixos.sh` | `scripts/nixos/nixos-install.sh` |
| `demo-nixos.sh` | *Integrated into nixos-install.sh* |
| `test-nix.sh` | `scripts/nixos/nixos-test.sh` |

**Use the main launcher for all operations:**
```bash
./scripts/bitchat-setup.sh <command>
```

This provides a unified, cross-platform interface that automatically detects your system and runs the appropriate scripts. 