# BitChat Linux - NixOS Setup Guide

Complete guide for running BitChat on NixOS with full Bluetooth LE mesh networking functionality.

## Quick Start

### Method 1: Flake (Recommended)

```bash
# Clone and run directly
git clone https://github.com/your-repo/bitchat-linux.git
cd bitchat-linux

# Run BitChat with full functionality
nix run

# Or build and install
nix build
./result/bin/bitchat-linux --verbose
```

### Method 2: NixOS System Service

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

Then rebuild your system:
```bash
sudo nixos-rebuild switch
```

## Complete NixOS Configuration

### Full System Configuration

```nix
# /etc/nixos/configuration.nix
{ config, pkgs, ... }:

{
  # Import BitChat module
  imports = [
    ./bitchat-linux/nixos-module.nix
  ];

  # Enable BitChat with full configuration
  services.bitchat = {
    enable = true;
    verbose = true;
    enableGui = false;        # Set to true for GUI mode
    enableNotifications = true;
    bluetoothAdapter = "hci0";
    
    extraConfig = {
      nickname = "NixOSMeshNode";
      battery_optimization = true;
      bluetooth = {
        advertisement_interval = 1000;
        scan_interval = 3000;
        max_connections = 12;
        connection_timeout = 30000;
      };
      channels = {
        joined = ["general" "nixos" "tech"];
        passwords = {
          "secure" = "mypassword123";
        };
      };
      ui = {
        show_system_tray = true;
        minimize_to_tray = true;
      };
      logging = {
        level = "info";
        file_enabled = true;
        console_enabled = true;
      };
    };
  };

  # Bluetooth configuration
  hardware.bluetooth = {
    enable = true;
    powerOnBoot = true;
    settings = {
      General = {
        Enable = "Source,Sink,Media,Socket";
        Experimental = true;
      };
      Policy = {
        AutoEnable = true;
      };
    };
  };

  # Additional system packages
  environment.systemPackages = with pkgs; [
    bluez-tools
    bluetuith    # TUI Bluetooth manager
  ];

  # Enable desktop environment (if using GUI)
  services.xserver = {
    enable = true;
    displayManager.gdm.enable = true;
    desktopManager.gnome.enable = true;
  };

  # Firewall (optional, for future WiFi Direct)
  networking.firewall = {
    enable = true;
    allowedTCPPorts = [ 8080 8443 ];
    allowedUDPPorts = [ 5353 ];
  };
}
```

### Home Manager Integration

If using Home Manager:

```nix
# ~/.config/nixpkgs/home.nix
{ config, pkgs, ... }:

{
  home.packages = with pkgs; [
    (pkgs.callPackage ./bitchat-linux/default.nix {})
  ];

  # Desktop entry
  xdg.desktopEntries.bitchat = {
    name = "BitChat";
    comment = "Decentralized mesh messaging";
    exec = "bitchat-linux";
    icon = "bitchat";
    categories = [ "Network" "InstantMessaging" ];
  };

  # Systemd user service
  systemd.user.services.bitchat = {
    Unit = {
      Description = "BitChat User Service";
      After = [ "bluetooth.service" ];
    };
    Service = {
      Type = "simple";
      ExecStart = "${pkgs.bitchat-linux}/bin/bitchat-linux --daemon";
      Restart = "always";
    };
    Install = {
      WantedBy = [ "default.target" ];
    };
  };
}
```

## Development Setup

### Development Environment

```bash
# Enter development shell
nix develop

# Or use legacy nix-shell
nix-shell

# Build and test
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Set capabilities for Bluetooth
sudo setcap cap_net_raw+eip ./bitchat-linux

# Run with full functionality
./bitchat-linux --verbose
```

### Available Development Commands

```bash
# Build variants
nix build                           # Release build
nix build .#bitchat-linux          # Explicit package
nix run                             # Run directly
nix run .#daemon                    # Run in daemon mode
nix run .#verbose                   # Run with verbose output

# Development tools
nix develop                         # Enter dev shell
nix develop --command gdb ./bitchat-linux  # Debug session
nix develop --command valgrind ./bitchat-linux  # Memory check
```

## Service Management

### System Service Control

```bash
# Start/stop service
sudo systemctl start bitchat
sudo systemctl stop bitchat
sudo systemctl restart bitchat

# Enable/disable at boot
sudo systemctl enable bitchat
sudo systemctl disable bitchat

# Check status
sudo systemctl status bitchat
sudo journalctl -u bitchat -f

# Health monitoring
sudo systemctl status bitchat-health
```

### Configuration Management

```bash
# View current configuration
sudo cat /var/lib/bitchat/.config/bitchat/config.json

# Edit configuration (restart required)
sudo systemctl edit bitchat

# View logs
sudo journalctl -u bitchat --since "1 hour ago"
```

## Bluetooth Setup

### Prerequisites

```bash
# Check Bluetooth adapter
sudo hciconfig -a

# Enable Bluetooth
sudo systemctl start bluetooth
sudo systemctl enable bluetooth

# Test Bluetooth functionality
bluetoothctl
> scan on
> devices
> quit
```

### Permissions and Capabilities

BitChat requires `CAP_NET_RAW` capability for Bluetooth LE access:

```bash
# Automatic (handled by NixOS module)
sudo setcap cap_net_raw+eip /run/current-system/sw/bin/bitchat-linux

# Manual capability setting
sudo setcap cap_net_raw+eip ./result/bin/bitchat-linux
```

### Troubleshooting Bluetooth

```bash
# Check Bluetooth status
sudo systemctl status bluetooth
rfkill list bluetooth

# Reset Bluetooth
sudo systemctl restart bluetooth
sudo hciconfig hci0 down
sudo hciconfig hci0 up

# Monitor Bluetooth traffic
sudo tcpdump -i bluetooth0
sudo btmon  # BlueZ monitor
```

## Application Usage

### GUI Mode

```bash
# Start GUI (requires X11/Wayland)
bitchat-linux

# With system tray
bitchat-linux --tray
```

### CLI Mode

```bash
# Interactive console
bitchat-linux --verbose

# Daemon mode
bitchat-linux --daemon

# With custom config
bitchat-linux --config /path/to/config.json
```

### Available Commands

```
/help                    # Show all commands
/status                  # Show connection status
/peers                   # List connected peers
/channels               # Show available channels
/join #channel          # Join a channel
/leave #channel         # Leave a channel
/msg @peer message      # Send private message
/nick nickname          # Change nickname
/block @peer            # Block a peer
/unblock @peer          # Unblock a peer
/clear                  # Clear message history
/quit                   # Exit application
```

## Configuration Options

### Complete Configuration Schema

```json
{
  "nickname": "BitChatUser",
  "encryption_enabled": true,
  "notifications_enabled": true,
  "dark_mode_enabled": false,
  "battery_optimization": true,
  "bluetooth": {
    "adapter": "hci0",
    "advertisement_interval": 1000,
    "scan_interval": 5000,
    "max_connections": 8,
    "connection_timeout": 30000,
    "enable_experimental": true
  },
  "channels": {
    "joined": ["general", "tech"],
    "passwords": {
      "secure": "password123"
    },
    "auto_join": true
  },
  "ui": {
    "show_system_tray": true,
    "minimize_to_tray": true,
    "close_to_tray": false,
    "theme": "system"
  },
  "logging": {
    "level": "info",
    "file_enabled": true,
    "console_enabled": true,
    "max_file_size": "10MB",
    "max_files": 5
  },
  "security": {
    "enable_encryption": true,
    "key_rotation_interval": 3600,
    "allow_anonymous": false
  },
  "performance": {
    "message_cache_size": 1000,
    "compression_enabled": true,
    "compression_threshold": 100
  }
}
```

### NixOS Module Options

```nix
services.bitchat = {
  enable = true;                    # Enable service
  package = pkgs.bitchat-linux;     # Package to use
  user = "bitchat";                 # Service user
  group = "bitchat";                # Service group
  dataDir = "/var/lib/bitchat";     # Data directory
  configFile = null;                # Custom config file
  extraConfig = {};                 # Additional config
  verbose = false;                  # Verbose logging
  enableGui = false;                # GUI mode
  enableNotifications = false;      # Desktop notifications
  bluetoothAdapter = "hci0";        # Bluetooth adapter
  extraArgs = [];                   # Additional CLI args
  openFirewall = false;             # Open firewall ports
};
```

## Security Considerations

### Capabilities and Permissions

- BitChat requires `CAP_NET_RAW` for Bluetooth LE access
- Service runs as dedicated `bitchat` user
- Strict systemd security settings applied
- No network access required (Bluetooth only)

### Encryption

- End-to-end encryption using libsodium
- X25519 key exchange for private messages
- AES-256-GCM for message encryption
- Ed25519 signatures for authenticity

### Privacy

- No persistent identifiers
- Local-only operation (no servers)
- Optional message retention
- Emergency data wipe capability

## Performance Tuning

### Battery Optimization

```nix
services.bitchat.extraConfig = {
  battery_optimization = true;
  bluetooth = {
    advertisement_interval = 2000;  # Slower advertising
    scan_interval = 10000;          # Less frequent scanning
    max_connections = 4;            # Fewer connections
  };
};
```

### High Performance

```nix
services.bitchat.extraConfig = {
  battery_optimization = false;
  bluetooth = {
    advertisement_interval = 500;   # Fast advertising
    scan_interval = 1000;           # Frequent scanning
    max_connections = 16;           # More connections
  };
  performance = {
    message_cache_size = 5000;      # Large cache
    compression_enabled = true;     # Enable compression
  };
};
```

## Monitoring and Debugging

### System Monitoring

```bash
# Service status
systemctl status bitchat

# Resource usage
systemctl show bitchat --property=CPUUsage,MemoryUsage

# Health checks
systemctl status bitchat-health
```

### Debug Logging

```bash
# Enable debug logging
sudo systemctl edit bitchat
# Add: Environment=BITCHAT_LOG_LEVEL=debug

# View debug logs
journalctl -u bitchat --since "1 hour ago" -f
```

### Network Debugging

```bash
# Monitor Bluetooth traffic
sudo btmon

# Check connections
ss -tuln | grep bitchat
lsof -p $(pgrep bitchat)

# Test connectivity
bluetoothctl scan on
hciconfig hci0 piscan
```

## Troubleshooting

### Common Issues

1. **Bluetooth not working**
   ```bash
   sudo systemctl restart bluetooth
   sudo hciconfig hci0 up
   rfkill unblock bluetooth
   ```

2. **Permission denied**
   ```bash
   sudo setcap cap_net_raw+eip /run/current-system/sw/bin/bitchat-linux
   ```

3. **Service fails to start**
   ```bash
   journalctl -u bitchat -n 50
   systemctl --user daemon-reload
   ```

4. **GUI not working**
   ```bash
   export DISPLAY=:0
   xhost +local:bitchat
   ```

### Getting Help

- Check logs: `journalctl -u bitchat -f`
- Test Bluetooth: `bluetoothctl scan on`
- Verify build: `nix build --show-trace`
- Debug mode: `bitchat-linux --verbose --debug`

## Advanced Configuration

### Custom Bluetooth Settings

```nix
hardware.bluetooth.settings = {
  General = {
    Enable = "Source,Sink,Media,Socket";
    Experimental = true;
    KernelExperimental = true;
  };
  LE = {
    EnableAdvMonInterleaveScan = true;
    EnableAdvMonInterleaveScanTimeout = 5;
  };
};
```

### Firewall Configuration

```nix
networking.firewall = {
  enable = true;
  allowedTCPPorts = [ 8080 8443 ];
  allowedUDPPorts = [ 5353 ];
  interfaces.bluetooth0.allowedTCPPorts = [ 8080 ];
};
```

### Multiple Instances

```nix
# Run multiple BitChat instances
services.bitchat-node1 = {
  enable = true;
  user = "bitchat1";
  dataDir = "/var/lib/bitchat1";
  bluetoothAdapter = "hci0";
  extraConfig.nickname = "Node1";
};

services.bitchat-node2 = {
  enable = true;
  user = "bitchat2";
  dataDir = "/var/lib/bitchat2";
  bluetoothAdapter = "hci1";
  extraConfig.nickname = "Node2";
};
```

This guide provides comprehensive coverage of BitChat setup and usage on NixOS. The application is now ready for production use with full mesh networking capabilities. 