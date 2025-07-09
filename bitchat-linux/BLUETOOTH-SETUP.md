# BitChat Bluetooth Setup Guide

This guide explains how to set up and run BitChat with real Bluetooth support outside of the container environment.

## Prerequisites

### System Requirements
- Linux system with Bluetooth LE support
- BlueZ Bluetooth stack
- sudo access for setting capabilities
- Modern Bluetooth hardware (4.0+)

### Software Requirements
- BlueZ (bluetoothctl)
- libcap (for setcap)
- Qt6 Core libraries
- libsodium (for encryption)

## Quick Setup

### 1. Exit Container and Navigate to Project

```bash
# Exit the container environment
exit

# Navigate to your BitChat directory
cd /path/to/your/bitchat/bitchat-linux
```

### 2. Use the Setup Script

The easiest way to set up Bluetooth support is using the provided setup script:

```bash
# Make script executable (if not already)
chmod +x setup-bluetooth.sh

# Test Bluetooth setup
./setup-bluetooth.sh --test

# Setup capabilities
./setup-bluetooth.sh --setup

# Run with Bluetooth support
./setup-bluetooth.sh --run --verbose
```

## Manual Setup

### 1. Check Bluetooth Availability

```bash
# Check if Bluetooth is available
hciconfig

# Check Bluetooth service
systemctl status bluetooth

# Enable Bluetooth if needed
sudo systemctl enable bluetooth
sudo systemctl start bluetooth
```

### 2. Set Required Capabilities

```bash
# Navigate to build directory
cd build

# Set capabilities for raw socket access
sudo setcap cap_net_raw,cap_net_admin+eip ./bitchat-linux

# Verify capabilities
getcap ./bitchat-linux
```

### 3. Test Bluetooth Functionality

```bash
# Test basic Bluetooth
bluetoothctl
> scan on
> devices
> quit
```

### 4. Run the Application

```bash
# Run with verbose output
./bitchat-linux --verbose

# Run in daemon mode
./bitchat-linux --daemon

# Run with custom config
./bitchat-linux --config ~/.config/bitchat/config.json
```

## Troubleshooting

### Permission Issues

**Problem**: `Error: bitchat requires CAP_NET_RAW capability`

**Solution**:
```bash
sudo setcap cap_net_raw,cap_net_admin+eip ./bitchat-linux
```

### Bluetooth Not Found

**Problem**: `qt.bluetooth.bluez: Missing CAP_NET_ADMIN permission`

**Solution**:
```bash
# Add user to bluetooth group
sudo usermod -a -G bluetooth $USER

# Log out and back in, or run:
newgrp bluetooth
```

### No Peers Discovered

**Problem**: Application runs but no peers are found

**Solutions**:
1. **Check Bluetooth range**: Ensure devices are within ~100m
2. **Verify other devices**: Make sure other devices are running BitChat
3. **Check Bluetooth state**: Ensure Bluetooth is enabled on all devices
4. **Check logs**: Look for Bluetooth errors in system logs

```bash
# Check Bluetooth logs
journalctl -u bluetooth -f

# Check if Bluetooth is working
bluetoothctl show
```

### Segmentation Fault

**Problem**: Application crashes with segmentation fault

**Solutions**:
1. **Check Qt platform**: Try different Qt platforms
2. **Run with offscreen platform**:
   ```bash
   QT_QPA_PLATFORM=offscreen ./bitchat-linux --verbose
   ```
3. **Check dependencies**: Ensure all Qt libraries are installed
4. **Run in debug mode**: Rebuild with debug symbols

## Advanced Configuration

### Custom Configuration

Create a configuration file at `~/.config/bitchat/config.json`:

```json
{
  "nickname": "MyDevice",
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
  }
}
```

### Systemd Service

Create a systemd service file at `/etc/systemd/system/bitchat.service`:

```ini
[Unit]
Description=BitChat Mesh Messaging
After=bluetooth.service
Wants=bluetooth.service

[Service]
Type=simple
User=your-username
ExecStart=/path/to/bitchat-linux --daemon
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start the service:

```bash
sudo systemctl enable bitchat
sudo systemctl start bitchat
sudo systemctl status bitchat
```

### Network Configuration

For optimal mesh networking:

```bash
# Check Bluetooth mesh support
sudo dmesg | grep -i bluetooth

# Load Bluetooth modules
sudo modprobe bluetooth
sudo modprobe btusb

# Check Bluetooth interface
hciconfig hci0 up
```

## Testing Bluetooth Functionality

### 1. Basic Test

```bash
# Test Bluetooth controller
bluetoothctl show

# Scan for devices
bluetoothctl scan on
```

### 2. BitChat Test

```bash
# Run BitChat in verbose mode
./bitchat-linux --verbose

# Look for these messages:
# - "Bluetooth controller available"
# - "Starting advertisement"
# - "Starting scan"
# - "Peer discovered"
```

### 3. Multi-Device Test

1. Run BitChat on multiple devices
2. Ensure devices are within Bluetooth range
3. Check if peers are discovered
4. Test message sending between devices

## Performance Optimization

### Battery Optimization

The application automatically adjusts power consumption:

- **Performance mode**: Full features when charging or >60% battery
- **Balanced mode**: Default operation (30-60% battery)
- **Power saver**: Reduced scanning when <30% battery
- **Ultra-low power**: Emergency mode when <10% battery

### Network Optimization

- **Message compression**: Automatic LZ4 compression for messages >100 bytes
- **Bloom filters**: Optimized duplicate detection
- **Adaptive scanning**: Duty cycle adjusts to battery state

## Security Considerations

### Capabilities

The application requires specific capabilities for Bluetooth LE:

- `CAP_NET_RAW`: Raw socket access for Bluetooth LE
- `CAP_NET_ADMIN`: Network administration for device discovery

### Encryption

- **Private messages**: X25519 key exchange + AES-256-GCM
- **Channel messages**: Argon2id password derivation + AES-256-GCM
- **Digital signatures**: Ed25519 for message authenticity

### Privacy

- **No registration**: No accounts or phone numbers required
- **Ephemeral by default**: Messages exist only in device memory
- **Local-first**: Works completely offline

## Troubleshooting Commands

```bash
# Check Bluetooth status
systemctl status bluetooth

# Check Bluetooth devices
hciconfig

# Check capabilities
getcap ./bitchat-linux

# Check Bluetooth logs
journalctl -u bluetooth -f

# Test Bluetooth functionality
bluetoothctl show

# Check Qt platform
echo $QT_QPA_PLATFORM

# Run with different platform
QT_QPA_PLATFORM=offscreen ./bitchat-linux --verbose
```

## Getting Help

If you encounter issues:

1. **Check logs**: Look for error messages in the application output
2. **Test Bluetooth**: Use `bluetoothctl` to verify Bluetooth functionality
3. **Check capabilities**: Ensure proper permissions are set
4. **Try mock mode**: Test with `--mock` flag to isolate Bluetooth issues
5. **Check dependencies**: Verify all required libraries are installed

## Next Steps

Once Bluetooth is working:

1. **Test with multiple devices**: Run BitChat on different devices
2. **Test mesh networking**: Verify messages relay through the network
3. **Test encryption**: Send private messages between devices
4. **Test channels**: Create and join channels with multiple devices
5. **Monitor performance**: Check battery usage and network efficiency 