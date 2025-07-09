# BitChat Linux Port

A complete Linux port of the BitChat decentralized mesh messaging application, originally designed for iOS/macOS. This port maintains full compatibility with the original protocol while adapting to Linux's ecosystem.

## Overview

BitChat Linux provides secure, decentralized messaging over Bluetooth Low Energy (BLE) mesh networks. No internet connection required - messages are routed through nearby devices creating a resilient communication network.

### Key Features

- **Decentralized Mesh Network**: Messages route through nearby devices
- **End-to-End Encryption**: All messages encrypted with libsodium (X25519 + AES-256-GCM)
- **Multi-Platform Protocol**: Compatible with iOS/macOS BitChat
- **Channel Support**: Join password-protected channels
- **Private Messaging**: Direct peer-to-peer communication
- **Battery Optimized**: Intelligent power management
- **GUI and CLI**: Both graphical and command-line interfaces
- **Daemon Mode**: Run as background service
- **System Integration**: Desktop notifications, system tray
- **NixOS Support**: Full NixOS integration with declarative configuration

## Technology Stack

### Core Technologies
- **Language**: C++17
- **UI Framework**: Qt6 (Widgets, Network, Bluetooth)
- **Networking**: Bluetooth LE via Qt6Bluetooth + BlueZ
- **Encryption**: libsodium (X25519, AES-256-GCM, Ed25519)
- **Compression**: LZ4
- **Notifications**: libnotify
- **Build System**: CMake

### Dependencies
- Qt6 (Core, Widgets, Network, Bluetooth)
- BlueZ development libraries
- libsodium
- liblz4
- libnotify
- libcap (for Bluetooth capabilities)

## Installation

### NixOS (Recommended)

#### Method 1: Flake
```bash
# Run directly
nix run github:your-repo/bitchat-linux

# Install to profile
nix profile install github:your-repo/bitchat-linux

# Build locally
git clone https://github.com/your-repo/bitchat-linux.git
cd bitchat-linux
nix build
```

#### Method 2: System Service
Add to your `/etc/nixos/configuration.nix`:

```nix
{
  # Import the BitChat module
  imports = [
    (builtins.fetchGit {
      url = "https://github.com/your-repo/bitchat-linux.git";
      ref = "main";
    } + "/nixos-module.nix")
  ];

  # Enable BitChat service
  services.bitchat = {
    enable = true;
    verbose = true;
    extraConfig = {
      nickname = "MyNixOSNode";
      bluetooth.max_connections = 10;
    };
  };

  # Enable Bluetooth
  hardware.bluetooth.enable = true;
}
```

Then rebuild: `sudo nixos-rebuild switch`

#### Method 3: Development
```bash
# Clone and enter development environment
git clone https://github.com/your-repo/bitchat-linux.git
cd bitchat-linux
nix develop

# Build and run
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
sudo setcap cap_net_raw+eip ./bitchat-linux
./bitchat-linux --verbose
```

See [README-NIXOS.md](README-NIXOS.md) for complete NixOS setup guide.

### Quick Install (Traditional Linux)

```bash
# Clone the repository
git clone https://github.com/your-repo/bitchat-linux.git
cd bitchat-linux

# Build and install with automatic dependency installation
./build.sh --install-deps --install

# Or build manually
./build.sh Release --install
```

### Manual Dependency Installation

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake pkg-config \
    qt6-base-dev qt6-bluetooth-dev \
    libbluetooth-dev libsodium-dev \
    liblz4-dev libnotify-dev libcap-dev
```

#### Fedora/RHEL
```bash
sudo dnf install -y \
    gcc-c++ cmake pkgconfig \
    qt6-qtbase-devel qt6-qtconnectivity-devel \
    bluez-libs-devel libsodium-devel \
    lz4-devel libnotify-devel libcap-devel
```

#### Arch Linux
```bash
sudo pacman -S --needed \
    base-devel cmake pkgconf \
    qt6-base qt6-connectivity \
    bluez libsodium lz4 libnotify libcap
```

### Building from Source

```bash
# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install
sudo make install

# Set Bluetooth capabilities
sudo setcap cap_net_raw+eip /usr/local/bin/bitchat-linux
```

## Usage

### GUI Mode (Default)
```bash
bitchat-linux                    # Start GUI
bitchat-linux --tray            # Start with system tray
```

### Command Line Interface
```bash
# Start with verbose output
bitchat-linux --verbose

# Run as daemon
bitchat-linux --daemon

# Custom configuration
bitchat-linux --config /path/to/config.json
```

### Available Commands (CLI)
- `/help` - Show available commands
- `/status` - Show connection status
- `/peers` - List connected peers
- `/channels` - Show available channels
- `/join #channel` - Join a channel
- `/leave #channel` - Leave a channel
- `/msg @peer message` - Send private message
- `/nick nickname` - Change nickname
- `/block @peer` - Block a peer
- `/unblock @peer` - Unblock a peer
- `/clear` - Clear message history
- `/quit` - Exit application

### Configuration

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

## System Integration

### Desktop Entry
The application installs a desktop entry for easy launching from your desktop environment.

### System Tray
When running in GUI mode, BitChat minimizes to the system tray and shows notifications for new messages.

### Systemd Service
Run as a system service:

```bash
# Install service
sudo systemctl enable bitchat
sudo systemctl start bitchat

# Check status
sudo systemctl status bitchat

# View logs
journalctl -u bitchat -f
```

### NixOS Service Management
```bash
# Enable in configuration.nix
services.bitchat.enable = true;

# Rebuild system
sudo nixos-rebuild switch

# Check status
systemctl status bitchat
journalctl -u bitchat -f
```

## Protocol Compatibility

This Linux port maintains full protocol compatibility with the original iOS/macOS BitChat:

### Message Format
- **Header**: Protocol version, message type, sender ID
- **Payload**: Encrypted message content
- **Signature**: Ed25519 signature for authenticity
- **Checksum**: CRC32 for integrity

### Encryption
- **Key Exchange**: X25519 Elliptic Curve Diffie-Hellman
- **Symmetric Encryption**: AES-256-GCM
- **Authentication**: Ed25519 signatures
- **Channel Keys**: PBKDF2 with SHA-256

### Network Protocol
- **Discovery**: BLE advertisement/scanning
- **Connection**: GATT-based communication
- **Routing**: Flood-based mesh with TTL
- **Reliability**: Automatic retransmission

## Security Features

### Encryption
- All messages encrypted end-to-end
- Forward secrecy with ephemeral keys
- Perfect forward secrecy for channels
- Authenticated encryption (AES-256-GCM)

### Privacy
- No persistent device identifiers
- Ephemeral connection IDs
- Optional message retention
- Local-only operation (no servers)

### Security Hardening
- Minimal attack surface
- Sandboxed execution (NixOS)
- Capability-based security
- Secure memory handling

## Performance Optimization

### Battery Management
- Adaptive scanning intervals
- Power-aware connection limits
- Background optimization
- Configurable power modes

### Network Efficiency
- Message compression (LZ4)
- Efficient routing algorithms
- Connection pooling
- Bandwidth optimization

### Memory Management
- Efficient message caching
- Automatic cleanup
- Memory-mapped storage
- Garbage collection

## Development

### Building for Development
```bash
# Traditional development
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# NixOS development
nix develop
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Testing
```bash
# Run test suite
./test_bitchat.sh

# Manual testing
./bitchat-linux --verbose --debug

# Bluetooth testing
bluetoothctl scan on
hciconfig hci0 up
```

### Debugging
```bash
# GDB debugging
gdb ./bitchat-linux

# Valgrind memory checking
valgrind --leak-check=full ./bitchat-linux

# System call tracing
strace -e bluetooth ./bitchat-linux
```

## Troubleshooting

### Common Issues

1. **Bluetooth Permission Denied**
   ```bash
   sudo setcap cap_net_raw+eip ./bitchat-linux
   ```

2. **Bluetooth Service Not Running**
   ```bash
   sudo systemctl start bluetooth
   sudo systemctl enable bluetooth
   ```

3. **Qt Platform Plugin Issues**
   ```bash
   export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/qt6/plugins
   ```

4. **Build Failures**
   ```bash
   # Check dependencies
   pkg-config --modversion qt6-bluetooth
   pkg-config --modversion libsodium
   ```

### NixOS Troubleshooting

1. **Service Won't Start**
   ```bash
   journalctl -u bitchat -n 50
   systemctl --user daemon-reload
   ```

2. **Bluetooth Issues**
   ```bash
   sudo systemctl restart bluetooth
   rfkill unblock bluetooth
   ```

3. **Build Issues**
   ```bash
   nix build --show-trace
   nix develop --command cmake --version
   ```

### Getting Help

- Check system logs: `journalctl -u bitchat -f`
- Test Bluetooth: `bluetoothctl scan on`
- Verify dependencies: `ldd ./bitchat-linux`
- Debug mode: `bitchat-linux --verbose --debug`

## Advanced Configuration

### Custom Bluetooth Settings
```json
{
  "bluetooth": {
    "adapter": "hci0",
    "advertisement_interval": 1000,
    "scan_interval": 5000,
    "max_connections": 8,
    "connection_timeout": 30000,
    "enable_experimental": true
  }
}
```

### Performance Tuning
```json
{
  "performance": {
    "message_cache_size": 1000,
    "compression_enabled": true,
    "compression_threshold": 100,
    "max_message_size": 4096
  }
}
```

### Security Settings
```json
{
  "security": {
    "enable_encryption": true,
    "key_rotation_interval": 3600,
    "allow_anonymous": false,
    "max_failed_attempts": 5
  }
}
```

## Packaging

### Creating Packages
```bash
# Debian/Ubuntu package
cpack -G DEB

# RPM package
cpack -G RPM

# Archive
cpack -G TGZ
```

### NixOS Package
```bash
# Build package
nix build

# Install to profile
nix profile install .

# Add to system
# Add to configuration.nix:
# environment.systemPackages = [ pkgs.bitchat-linux ];
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Development Environment
```bash
# NixOS (recommended)
nix develop

# Traditional
./build.sh --install-deps
```

### Code Style
- Follow existing C++ style
- Use clang-format
- Add tests for new features
- Update documentation

## License

This project is released into the public domain. See the [LICENSE](LICENSE) file for details.

## Links

- **Main Repository**: https://github.com/your-repo/bitchat-linux
- **NixOS Guide**: [README-NIXOS.md](README-NIXOS.md)
- **Technical Whitepaper**: [WHITEPAPER.md](WHITEPAPER.md)
- **Issues**: https://github.com/your-repo/bitchat-linux/issues

BitChat Linux brings secure, decentralized messaging to the Linux desktop with full cross-platform compatibility and native system integration. 