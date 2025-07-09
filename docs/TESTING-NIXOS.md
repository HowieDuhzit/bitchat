# Testing BitChat on NixOS

This guide covers how to test the BitChat Linux port on NixOS, including development setup, building, and deployment.

## Quick Start

### Method 1: Development Shell (Recommended for Testing)

1. **Enter the development environment:**
   ```bash
   nix-shell
   ```

2. **Build the project:**
   ```bash
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j$(nproc)
   ```

3. **Test the application:**
   ```bash
   # Make sure Bluetooth is enabled
   sudo systemctl start bluetooth
   
   # Run BitChat
   ./bitchat-linux --verbose
   ```

### Method 2: Build and Install Package

1. **Build the package:**
   ```bash
   nix-build
   ```

2. **Install temporarily:**
   ```bash
   nix-env -i ./result
   ```

3. **Set capabilities and run:**
   ```bash
   sudo setcap cap_net_raw+eip ~/.nix-profile/bin/bitchat-linux
   bitchat-linux
   ```

### Method 3: System-wide Installation (NixOS)

1. **Add to your NixOS configuration:**
   ```nix
   # /etc/nixos/configuration.nix
   { config, pkgs, ... }:
   
   let
     bitchat = pkgs.callPackage /path/to/bitchat/default.nix {};
   in
   {
     imports = [
       /path/to/bitchat/nixos-module.nix
     ];
     
     services.bitchat = {
       enable = true;
       verbose = true;
     };
     
     # Ensure Bluetooth is enabled
     hardware.bluetooth.enable = true;
     
     # Add to system packages for GUI access
     environment.systemPackages = [ bitchat ];
   }
   ```

2. **Rebuild your system:**
   ```bash
   sudo nixos-rebuild switch
   ```

3. **Check service status:**
   ```bash
   sudo systemctl status bitchat
   journalctl -u bitchat -f
   ```

## Prerequisites

### System Requirements

- NixOS with Bluetooth support
- Bluetooth adapter (built-in or USB)
- Qt6 support (handled by Nix)

### Enable Bluetooth

Add to your NixOS configuration:
```nix
hardware.bluetooth = {
  enable = true;
  powerOnBoot = true;
};

services.blueman.enable = true;  # Optional: Bluetooth manager GUI
```

Then rebuild: `sudo nixos-rebuild switch`

## Development Workflow

### 1. Set up Development Environment

```bash
# Clone the repository
git clone <repository-url>
cd bitchat-linux

# Enter development shell
nix-shell

# The shell will automatically:
# - Install all dependencies
# - Set up environment variables
# - Create build directory
# - Display helpful information
```

### 2. Build and Test

```bash
# Configure build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
make -j$(nproc)

# Test basic functionality
./bitchat-linux --help

# Test with verbose output
./bitchat-linux --verbose
```

### 3. Debug Build Issues

If you encounter build issues:

```bash
# Clean build
rm -rf build
mkdir build
cd build

# Verbose CMake configuration
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON

# Check dependencies
pkg-config --list-all | grep -E "(qt6|bluez|sodium|lz4|notify)"
```

## Testing Scenarios

### 1. Basic Functionality Test

```bash
# Terminal 1: Start first instance
./bitchat-linux --verbose

# Terminal 2: Start second instance (different config)
./bitchat-linux --verbose --config /tmp/bitchat-test.json
```

### 2. Bluetooth Connectivity Test

```bash
# Check Bluetooth status
bluetoothctl show
bluetoothctl scan on

# Run BitChat with Bluetooth debugging
./bitchat-linux --verbose 2>&1 | grep -i bluetooth
```

### 3. GUI Testing

```bash
# Test GUI mode
./bitchat-linux

# Test with different Qt themes
QT_STYLE_OVERRIDE=Fusion ./bitchat-linux
```

### 4. Daemon Mode Testing

```bash
# Start as daemon
./bitchat-linux --daemon --verbose

# Check if running
ps aux | grep bitchat

# Test configuration
./bitchat-linux --daemon --config /path/to/config.json
```

## NixOS-Specific Testing

### 1. Test NixOS Module

Create a test configuration:
```nix
# test-bitchat.nix
{ config, pkgs, ... }:
{
  imports = [ ./nixos-module.nix ];
  
  services.bitchat = {
    enable = true;
    verbose = true;
    user = "bitchat-test";
    group = "bitchat-test";
  };
  
  hardware.bluetooth.enable = true;
}
```

Test with:
```bash
nixos-rebuild build-vm -I nixos-config=test-bitchat.nix
```

### 2. Test Capabilities

```bash
# Check if capabilities are set correctly
getcap /nix/store/*/bin/bitchat-linux

# Test capability inheritance
sudo -u bitchat-test /nix/store/*/bin/bitchat-linux --verbose
```

### 3. Test Service Integration

```bash
# Check service definition
systemctl cat bitchat

# Test service start/stop
sudo systemctl start bitchat
sudo systemctl status bitchat
sudo systemctl stop bitchat

# Check logs
journalctl -u bitchat --no-pager
```

## Troubleshooting

### Common Issues

1. **Bluetooth Permission Denied**
   ```bash
   # Check capabilities
   getcap ./bitchat-linux
   
   # Set capabilities manually
   sudo setcap cap_net_raw+eip ./bitchat-linux
   ```

2. **Qt6 Not Found**
   ```bash
   # Ensure you're in nix-shell
   echo $QT_QPA_PLATFORM_PLUGIN_PATH
   
   # If not set, run:
   nix-shell
   ```

3. **BlueZ Issues**
   ```bash
   # Check BlueZ service
   systemctl status bluetooth
   
   # Restart if needed
   sudo systemctl restart bluetooth
   ```

4. **CMake Configuration Issues**
   ```bash
   # Check CMake can find Qt6
   cmake .. -DCMAKE_BUILD_TYPE=Debug --debug-find
   
   # Check pkg-config paths
   echo $PKG_CONFIG_PATH
   ```

### Debug Commands

```bash
# Check all dependencies
nix-shell --run "pkg-config --list-all | grep -E '(qt6|bluez|sodium|lz4|notify)'"

# Test Qt6 installation
nix-shell --run "qmake6 -query"

# Check library paths
nix-shell --run "echo \$LD_LIBRARY_PATH"

# Test Bluetooth
nix-shell --run "bluetoothctl show"
```

### Performance Testing

```bash
# Memory usage
valgrind --tool=memcheck ./bitchat-linux --daemon

# CPU profiling
perf record ./bitchat-linux --verbose
perf report

# Network activity
sudo tcpdump -i bluetooth0
```

## Automated Testing

### Unit Tests (if implemented)

```bash
# Build tests
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
make -j$(nproc)

# Run tests
ctest --verbose
```

### Integration Tests

```bash
# Test script for basic functionality
#!/bin/bash
set -e

echo "Testing BitChat on NixOS..."

# Build
nix-build

# Test help
./result/bin/bitchat-linux --help

# Test configuration
./result/bin/bitchat-linux --config /dev/null --daemon &
BITCHAT_PID=$!

sleep 5

# Check if running
if kill -0 $BITCHAT_PID 2>/dev/null; then
    echo "✓ BitChat daemon started successfully"
    kill $BITCHAT_PID
else
    echo "✗ BitChat daemon failed to start"
    exit 1
fi

echo "All tests passed!"
```

## Deployment Options

### 1. User Installation

```bash
# Install for current user
nix-env -i ./result

# Or use nix profile (newer)
nix profile install .
```

### 2. System Installation

Add to `/etc/nixos/configuration.nix`:
```nix
environment.systemPackages = [
  (pkgs.callPackage /path/to/bitchat/default.nix {})
];
```

### 3. Flake-based Installation

Create `flake.nix`:
```nix
{
  description = "BitChat Linux";
  
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  
  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      packages.${system}.default = pkgs.callPackage ./default.nix {};
      
      nixosModules.default = import ./nixos-module.nix;
      
      devShells.${system}.default = import ./shell.nix { inherit pkgs; };
    };
}
```

Then install with:
```bash
nix build
nix run
```

## Contributing

When contributing to BitChat on NixOS:

1. Test in `nix-shell` first
2. Ensure the package builds with `nix-build`
3. Test the NixOS module if making system-level changes
4. Update documentation for any new dependencies
5. Test on multiple NixOS versions if possible

## Support

For NixOS-specific issues:
- Check the NixOS manual for Bluetooth configuration
- Use `nixos-option` to check current configuration
- Test with `nixos-rebuild build-vm` for safe testing
- Check the NixOS discourse for community support 