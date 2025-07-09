# BitChat Linux

A complete 1:1 port of the BitChat iOS decentralized messaging app for Linux systems.

## Overview

BitChat Linux is a decentralized, peer-to-peer messaging application that operates entirely over Bluetooth mesh networks. No internet connection, servers, or central infrastructure required. This is a feature-complete port of the original iOS BitChat application, maintaining 100% protocol compatibility for cross-platform communication.

## Features

### Core Messaging
- **Decentralized Messaging**: Pure peer-to-peer communication over Bluetooth
- **Cross-Platform Compatibility**: Full interoperability with iOS BitChat app
- **Real-time Sync**: Instant message delivery across the mesh network
- **Message Persistence**: Encrypted local storage for favorite channels
- **Offline Capability**: Works completely offline with nearby devices

### Network & Protocol
- **Bluetooth Mesh Networking**: Automatic peer discovery and mesh formation
- **Binary Protocol**: Efficient iOS-compatible binary message format
- **Message Routing**: Intelligent TTL-based message forwarding
- **Duplicate Detection**: Optimized Bloom filter for message deduplication
- **Fragmentation Support**: Large message splitting and reassembly

### Security & Privacy
- **End-to-End Encryption**: libsodium-based message encryption
- **Secure Key Storage**: Encrypted keychain management
- **Private Channels**: Password-protected channel communication
- **Ephemeral Peer IDs**: Session-based peer identification

### Reliability & Performance
- **Message Retry Logic**: Automatic retry with exponential backoff
- **Delivery Tracking**: Confirmation and read receipt system
- **Battery Optimization**: Adaptive scanning and power management
- **Network Scaling**: Probabilistic flooding for large networks

## Architecture

The application follows the exact same architecture as the iOS version:

```
bitchat-linux/
├── Services/           # Core business logic services
├── Utils/             # Utility classes and helpers
├── Protocols/         # Network protocol implementations
├── ViewModels/        # Application state management
├── Views/             # User interface components
└── Assets/            # Resources and configuration
```

### Services
- **BluetoothMeshService**: Bluetooth mesh networking and peer management
- **MessageRetryService**: Message retry logic and queue management
- **DeliveryTracker**: Delivery confirmation and status tracking
- **MessageRetentionService**: Encrypted message storage for favorites
- **KeychainManager**: Secure credential and key management
- **NotificationService**: System notification integration
- **EncryptionService**: End-to-end encryption implementation

### Utils
- **OptimizedBloomFilter**: Efficient duplicate message detection
- **BatteryOptimizer**: Power management and optimization
- **CompressionUtil**: Message payload compression

### Protocols
- **BinaryProtocol**: iOS-compatible binary message format
- **BitchatProtocol**: Core protocol definitions and structures

## Installation

### Prerequisites

- Qt6 (Core, Widgets, Bluetooth)
- libsodium (encryption)
- zlib (compression)
- BlueZ (Bluetooth stack)
- GLib and D-Bus (system integration)

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-bluetooth-dev libsodium-dev zlib1g-dev \
                 libglib2.0-dev libdbus-1-dev libcap-dev cmake build-essential
```

### Fedora/RHEL
```bash
sudo dnf install qt6-qtbase-devel qt6-qtconnectivity-devel libsodium-devel \
                 zlib-devel glib2-devel dbus-devel libcap-devel cmake gcc-c++
```

### Build
```bash
cd bitchat-linux
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

## Usage

### Starting the Application
```bash
bitchat-linux
```

### Basic Commands
- **Send Message**: Type and press Enter
- **Private Message**: Click on a peer name to start private chat
- **Channel Management**: Use `/join #channel` to join channels
- **Nickname**: Set your display name in settings

### Channel Commands
- `/join #channel` - Join a channel
- `/leave #channel` - Leave a channel
- `/channels` - List available channels
- `/peers` - Show connected peers
- `/status` - Display connection status

## Protocol Compatibility

This Linux implementation maintains 100% protocol compatibility with the iOS version:

- **Service UUID**: `F47B5E2D-4A9E-4C5A-9B3F-8E1D2C3A4B5C`
- **Characteristic UUID**: `A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D`
- **Protocol Version**: 1
- **Binary Format**: Big-endian, 13-byte header + 8-byte sender ID
- **Message Types**: All iOS message types supported
- **Encryption**: Compatible libsodium implementation

## Configuration

### Bluetooth Permissions
```bash
# Add user to bluetooth group
sudo usermod -a -G bluetooth $USER

# Set capabilities for raw socket access
sudo setcap cap_net_raw+ep /usr/local/bin/bitchat-linux
```

### System Integration
Desktop file and icon are automatically installed to:
- Desktop file: `/usr/share/applications/bitchat.desktop`
- Icon: `/usr/share/icons/hicolor/256x256/apps/bitchat.png`

## Development

### Building from Source
```bash
git clone https://github.com/your-repo/bitchat-linux.git
cd bitchat-linux
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Running Tests
```bash
# Unit tests
make test

# Integration tests
./test-integration.sh
```

### Code Style
The codebase follows Qt coding conventions and maintains consistency with the iOS Swift implementation patterns.

## Troubleshooting

### Bluetooth Issues
- Ensure BlueZ is running: `sudo systemctl status bluetooth`
- Check adapter status: `bluetoothctl show`
- Verify permissions: User must be in `bluetooth` group

### Connection Problems
- Check firewall settings for Bluetooth
- Verify Qt6 Bluetooth module installation
- Ensure libsodium is properly linked

### Performance Issues
- Adjust scanning intervals in configuration
- Monitor battery optimization settings
- Check system resource usage

## Contributing

1. Fork the repository
2. Create a feature branch
3. Follow the existing code style
4. Ensure iOS compatibility
5. Add tests for new features
6. Submit a pull request

## License

This project is released into the public domain under the Unlicense.
See the LICENSE file for details.

## Acknowledgments

- Original iOS BitChat application
- Qt Project for the excellent framework
- libsodium for cryptographic functions
- BlueZ project for Linux Bluetooth support

## Support

For issues, questions, or contributions:
- GitHub Issues: [Report bugs and request features]
- Documentation: [Wiki and guides]
- Community: [Discussion forums] 