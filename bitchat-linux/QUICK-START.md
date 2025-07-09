# BitChat Quick Start Guide

This guide shows you how to run BitChat in different modes and environments.

## 🚀 Quick Options

### Option 1: Terminal Mock Mode (Recommended for Testing)
```bash
# Run the simple terminal version (no compilation needed)
./bitchat-terminal.sh --mock
```

### Option 2: Use Setup Script (Outside Container)
```bash
# Test Bluetooth setup
./setup-bluetooth.sh --test

# Setup capabilities and run with Bluetooth
./setup-bluetooth.sh --run --verbose
```

### Option 3: Manual Bluetooth Setup (Outside Container)
```bash
# Set capabilities
sudo setcap cap_net_raw,cap_net_admin+eip build/bitchat-linux

# Run with Bluetooth
./build/bitchat-linux --verbose
```

## 📋 What We've Created

### ✅ Working Terminal Version
- **File**: `bitchat-terminal.sh`
- **Features**: Pure terminal interface, mock mode, no dependencies
- **Usage**: `./bitchat-terminal.sh --mock`

### ✅ Bluetooth Setup Script
- **File**: `setup-bluetooth.sh`
- **Features**: Automated Bluetooth setup, capability management
- **Usage**: `./setup-bluetooth.sh --run --verbose`

### ✅ Comprehensive Documentation
- **File**: `BLUETOOTH-SETUP.md`
- **Features**: Complete setup guide, troubleshooting, advanced config
- **File**: `README-TERMINAL.md`
- **Features**: Terminal version documentation

## 🎯 Recommended Workflow

### For Development/Testing (In Container)
```bash
# Use the terminal mock version
./bitchat-terminal.sh --mock

# Test commands
/help
/j general
Hello world!
/status
/quit
```

### For Production (Outside Container)
```bash
# Exit container first
exit

# Navigate to project directory
cd /path/to/bitchat/bitchat-linux

# Use setup script
./setup-bluetooth.sh --run --verbose
```

## 🔧 Available Commands

### Basic Commands
- `/help` - Show all commands
- `/j #channel` - Join a channel
- `/m @name message` - Send private message
- `/w` - List online users
- `/nick nickname` - Change nickname
- `/status` - Show connection status
- `/quit` - Exit application

### Examples
```bash
> /j general
Joined channel: #general
> Hello everyone!
[User123] Hello everyone!
> /m @alice Hi Alice!
[User123 -> alice] Hi Alice!
> /status
Connection Status:
  Nickname: User123
  Mode: Mock (no Bluetooth)
  Peers connected: 0
  Channels joined: 1
  Status: Running
```

## 🛠️ Troubleshooting

### Container Environment Issues
- **Problem**: Can't set Bluetooth capabilities
- **Solution**: Use `bitchat-terminal.sh --mock`

### Bluetooth Permission Issues
- **Problem**: `Error: bitchat requires CAP_NET_RAW capability`
- **Solution**: Use `setup-bluetooth.sh --setup`

### Qt Platform Issues
- **Problem**: `qt.qpa.plugin: Could not find the Qt platform plugin`
- **Solution**: Use `QT_QPA_PLATFORM=offscreen ./bitchat-linux`

## 📁 File Structure

```
bitchat-linux/
├── bitchat-terminal.sh          # Simple terminal version (working)
├── setup-bluetooth.sh          # Bluetooth setup script
├── build/
│   └── bitchat-linux           # Compiled binary
├── BLUETOOTH-SETUP.md          # Complete Bluetooth guide
├── README-TERMINAL.md          # Terminal version docs
└── QUICK-START.md              # This file
```

## 🎉 Success Indicators

### Terminal Mock Mode
- ✅ Application starts without errors
- ✅ Commands work (`/help`, `/j #channel`, etc.)
- ✅ Messages display correctly
- ✅ Status shows mock mode

### Bluetooth Mode
- ✅ No capability errors
- ✅ Bluetooth service running
- ✅ Application discovers peers
- ✅ Messages relay through mesh

## 🚀 Next Steps

1. **Test the terminal version** in the container
2. **Exit the container** when ready for Bluetooth
3. **Use the setup script** for easy Bluetooth configuration
4. **Test with multiple devices** for mesh networking
5. **Monitor performance** and battery usage

## 📞 Getting Help

- **Terminal issues**: Check `README-TERMINAL.md`
- **Bluetooth issues**: Check `BLUETOOTH-SETUP.md`
- **Build issues**: Use the terminal mock version
- **Permission issues**: Use the setup script

The terminal version provides a working chat interface that you can use immediately for testing and development! 