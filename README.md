![ChatGPT Image Jul 5, 2025 at 06_07_31 PM](https://github.com/user-attachments/assets/2660f828-49c7-444d-beca-d8b01854667a)
# bitchat

A secure, decentralized, peer-to-peer messaging app that works over Bluetooth mesh networks. No internet required, no servers, no phone numbers - just pure encrypted communication.

**Now available on Linux!** 🐧 Full cross-platform compatibility with native Linux support.

## License

This project is released into the public domain. See the [LICENSE](LICENSE) file for details.

## Features

- **Cross-Platform**: Native support for iOS, macOS, and Linux with 100% protocol compatibility
- **Decentralized Mesh Network**: Automatic peer discovery and multi-hop message relay over Bluetooth LE
- **End-to-End Encryption**: X25519 key exchange + AES-256-GCM for private messages
- **Channel-Based Chats**: Topic-based group messaging with optional password protection
- **Store & Forward**: Messages automatically relay through the mesh to reach distant peers
- **No Registration**: No accounts, emails, phone numbers, or servers required
- **Battery Optimized**: Intelligent scanning and connection management
- **Ephemeral by Default**: Messages exist only in device memory unless explicitly saved

## Quick Start

### 🚀 Universal Setup (All Platforms)

Use the main script launcher for all operations:

```bash
# Clone the repository
git clone https://github.com/your-repo/bitchat.git
cd bitchat

# Setup development environment (auto-detects platform)
./scripts/bitchat-setup.sh setup

# Build for your platform
./scripts/bitchat-setup.sh build

# Run tests
./scripts/bitchat-setup.sh test

# Install system-wide
./scripts/bitchat-setup.sh install
```

### 📱 iOS/macOS

```bash
# Setup Xcode project
./scripts/bitchat-setup.sh setup --ios

# Then open in Xcode
open bitchat.xcodeproj
```

### 🐧 Linux (Standard Distributions)

```bash
# Build with automatic dependency installation
./scripts/build/linux-build.sh --install-deps --install

# Run BitChat
bitchat-linux --verbose
```

### ❄️ NixOS

```bash
# Install system-wide service
./scripts/nixos/nixos-install.sh

# Or use flakes
nix run github:your-repo/bitchat#bitchat-linux
```

## Detailed Setup Instructions

### iOS/macOS Setup

#### Option 1: Using XcodeGen (Recommended)

1. Install XcodeGen:
   ```bash
   brew install xcodegen
   ```

2. Generate and open the project:
   ```bash
   ./scripts/setup/ios-setup.sh
   ```

#### Option 2: Using Swift Package Manager

1. Open the project in Xcode:
   ```bash
   cd bitchat
   open Package.swift
   ```

2. Select your target device and run

### Linux Setup

#### Quick Install

```bash
# Clone and build with one command
git clone https://github.com/your-repo/bitchat.git
cd bitchat

# Build with automatic dependency installation
./scripts/build/linux-build.sh --install-deps --install

# Run BitChat
bitchat-linux --verbose
```

#### Advanced Build Options

```bash
# Debug build with verbose output
./scripts/build/linux-build.sh --debug --verbose --clean

# Release build with system installation
./scripts/build/linux-build.sh --install-deps --install

# Custom build directory
./scripts/build/linux-build.sh --clean --verbose
```

#### NixOS (Recommended for Linux)

```bash
# Quick test
nix-build && ./result/bin/bitchat-linux --help

# Install system service
./scripts/nixos/nixos-install.sh

# Or add to your NixOS configuration
services.bitchat.enable = true;
```

See [bitchat-linux/README.md](bitchat-linux/README.md) for complete NixOS setup guide.

#### Manual Build (Linux)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential cmake pkg-config \
    qt6-base-dev qt6-bluetooth-dev \
    libbluetooth-dev libsodium-dev \
    liblz4-dev libnotify-dev libcap-dev

# Build
cd bitchat-linux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Set Bluetooth capabilities
sudo setcap cap_net_raw+eip ./bitchat-linux

# Run
./bitchat-linux --verbose
```

## Testing

### Comprehensive Testing

```bash
# Run tests for your platform
./scripts/bitchat-setup.sh test

# Linux-specific comprehensive tests
./scripts/test/linux-test.sh

# NixOS-specific tests
./scripts/nixos/nixos-test.sh
```

### Check Project Status

```bash
# View project status and available scripts
./scripts/bitchat-setup.sh status
```

## Scripts Organization

All scripts are now organized in the `scripts/` directory:

- **`scripts/bitchat-setup.sh`** - Main launcher (use this for everything!)
- **`scripts/setup/`** - Platform setup scripts
- **`scripts/build/`** - Build scripts
- **`scripts/test/`** - Test scripts
- **`scripts/nixos/`** - NixOS-specific scripts

See [scripts/README.md](scripts/README.md) for detailed documentation.

## Cross-Platform Compatibility

The Linux implementation maintains **100% protocol compatibility** with iOS/macOS:

- ✅ **Same Service UUIDs**: F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C
- ✅ **Same Binary Protocol**: 13-byte header, big-endian format
- ✅ **Same Message Types**: All iOS message types supported
- ✅ **Same Encryption**: X25519 + AES-256-GCM with libsodium
- ✅ **Same Features**: Channels, private messages, delivery tracking
- ✅ **Cross-Platform Messaging**: iOS ↔ Linux communication verified

See [bitchat-linux/FEATURE_PARITY.md](bitchat-linux/FEATURE_PARITY.md) for detailed comparison.

## Usage

### Basic Commands (All Platforms)

- `/help` - Show all available commands
- `/j #channel` - Join or create a channel
- `/m @name message` - Send a private message
- `/w` - List online users
- `/channels` - Show all discovered channels
- `/peers` - List connected peers
- `/nick nickname` - Change your nickname
- `/block @name` - Block a peer from messaging you
- `/unblock @name` - Unblock a peer
- `/clear` - Clear chat messages
- `/status` - Show connection status
- `/quit` - Exit application

### Channel Management

- `/pass [password]` - Set/change channel password (owner only)
- `/transfer @name` - Transfer channel ownership
- `/save` - Toggle message retention for channel (owner only)
- `/leave #channel` - Leave a channel

### Linux-Specific Features

#### GUI Mode
```bash
bitchat-linux                    # Start GUI
bitchat-linux --tray            # Start with system tray
```

#### CLI Mode
```bash
bitchat-linux --verbose         # Interactive console
bitchat-linux --daemon          # Background daemon
bitchat-linux --config /path/to/config.json  # Custom config
```

#### System Service (Linux)
```bash
# Install as systemd service
sudo systemctl enable bitchat
sudo systemctl start bitchat

# View logs
journalctl -u bitchat -f
```

### Getting Started

1. Launch bitchat on your device
2. Set your nickname (or use the auto-generated one)
3. You'll automatically connect to nearby peers
4. Join a channel with `/j #general` or start chatting in public
5. Messages relay through the mesh network to reach distant peers

### Channel Features

- **Password Protection**: Channel owners can set passwords with `/pass`
- **Message Retention**: Owners can enable mandatory message saving with `/save`
- **@ Mentions**: Use `@nickname` to mention users (with autocomplete)
- **Ownership Transfer**: Pass control to trusted users with `/transfer`

## Configuration

### Linux Configuration

Configuration is stored in `~/.config/bitchat/config.json`:

```json
{
  "nickname": "your_nickname",
  "encryption_enabled": true,
  "notifications_enabled": true,
  "battery_optimization": true,
  "bluetooth": {
    "advertisement_interval": 1000,
    "scan_interval": 5000,
    "max_connections": 8
  },
  "channels": {
    "joined": ["general", "tech"],
    "passwords": {}
  },
  "ui": {
    "show_system_tray": true,
    "minimize_to_tray": true
  }
}
```

### NixOS Configuration

```nix
services.bitchat = {
  enable = true;
  verbose = true;
  extraConfig = {
    nickname = "MyNode";
    bluetooth.max_connections = 10;
    channels.joined = ["general" "tech"];
  };
};
```

## Security & Privacy

### Encryption
- **Private Messages**: X25519 key exchange + AES-256-GCM encryption
- **Channel Messages**: Argon2id password derivation + AES-256-GCM
- **Digital Signatures**: Ed25519 for message authenticity
- **Forward Secrecy**: New key pairs generated each session

### Privacy Features
- **No Registration**: No accounts, emails, or phone numbers required
- **Ephemeral by Default**: Messages exist only in device memory
- **Cover Traffic**: Random delays and dummy messages prevent traffic analysis
- **Emergency Wipe**: Triple-tap logo to instantly clear all data (iOS/macOS)
- **Local-First**: Works completely offline, no servers involved

## Performance & Efficiency

### Message Compression
- **LZ4 Compression**: Automatic compression for messages >100 bytes
- **30-70% bandwidth savings** on typical text messages
- **Smart compression**: Skips already-compressed data

### Battery Optimization
- **Adaptive Power Modes**: Automatically adjusts based on battery level
  - Performance mode: Full features when charging or >60% battery
  - Balanced mode: Default operation (30-60% battery)
  - Power saver: Reduced scanning when <30% battery
  - Ultra-low power: Emergency mode when <10% battery
- **Background efficiency**: Automatic power saving when app backgrounded
- **Configurable scanning**: Duty cycle adapts to battery state

### Network Efficiency
- **Optimized Bloom filters**: Faster duplicate detection with less memory
- **Message aggregation**: Batches small messages to reduce transmissions
- **Adaptive connection limits**: Adjusts peer connections based on power mode

## Technical Architecture

### Cross-Platform Protocol
BitChat uses a unified binary protocol across all platforms:
- **iOS/macOS**: Swift implementation with Core Bluetooth
- **Linux**: C++ implementation with Qt6 Bluetooth and BlueZ
- **Full Compatibility**: All platforms can communicate seamlessly

### Binary Protocol
- Compact packet format with 1-byte type field
- TTL-based message routing (max 7 hops)
- Automatic fragmentation for large messages
- Message deduplication via unique IDs

### Mesh Networking
- Each device acts as both client and peripheral
- Automatic peer discovery and connection management
- Store-and-forward for offline message delivery
- Adaptive duty cycling for battery optimization

For detailed protocol documentation, see the [Technical Whitepaper](WHITEPAPER.md).

## Platform-Specific Documentation

- **iOS/macOS**: See main README sections above
- **Linux**: [README-Linux.md](README-Linux.md) - Complete Linux setup guide
- **NixOS**: [README-NIXOS.md](README-NIXOS.md) - NixOS integration guide

## Building for Production

### iOS/macOS
1. Set your development team in project settings
2. Configure code signing
3. Archive and distribute through App Store or TestFlight

### Linux
```bash
# Release build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install system-wide
sudo make install

# Create package (Debian/Ubuntu)
cpack -G DEB

# NixOS package
nix build
```

## System Integration

### Linux Desktop Integration
- **Desktop Entry**: Appears in application menus
- **System Tray**: Minimizes to system tray
- **Notifications**: Native desktop notifications
- **Systemd Service**: Can run as system service
- **D-Bus Integration**: Proper desktop integration

### NixOS Integration
- **Declarative Configuration**: Full NixOS module
- **Service Management**: Systemd integration
- **Security**: Proper capabilities and sandboxing
- **Monitoring**: Health checks and logging

## Development

### Requirements
- **iOS/macOS**: Xcode 15+, iOS 16.0+, macOS 13.0+
- **Linux**: C++17, Qt6, CMake, BlueZ, libsodium

### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on multiple platforms
5. Submit a pull request

### Testing
```bash
# iOS/macOS: Use Xcode testing
# Linux: 
cd build
make test
./test_bitchat.sh
```

## Compatibility

The protocol is designed to be platform-agnostic. Additional clients can be built for:
- **Android**: Using Android Bluetooth LE APIs
- **Windows**: Using Windows Bluetooth APIs
- **Web**: Using Web Bluetooth API (limited)

All platforms use the same packet structure, encryption, and service/characteristic UUIDs for full interoperability.

## Future Roadmap

- **WiFi Direct Support**: Extend beyond Bluetooth LE
- **Android Client**: Native Android implementation
- **Web Client**: Browser-based client with Web Bluetooth
- **Improved GUI**: Enhanced user interface
- **Plugin System**: Extensible architecture
- **Advanced Routing**: Improved mesh algorithms

## Community

- **Issues**: Report bugs and feature requests
- **Discussions**: Join community discussions
- **Documentation**: Help improve documentation
- **Testing**: Test on different devices and platforms

BitChat is designed to be a truly decentralized, privacy-first messaging platform that works anywhere, anytime, without relying on centralized infrastructure.
