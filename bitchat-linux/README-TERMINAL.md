# BitChat Terminal

A pure terminal version of BitChat that provides a simple, lightweight interface for decentralized mesh messaging.

## Features

- **Pure Terminal Interface**: No GUI dependencies, works in any terminal
- **Mock Mode**: Test functionality without Bluetooth hardware
- **Simple Commands**: Easy-to-use slash commands
- **Cross-Platform**: Works on any Unix-like system with bash
- **Lightweight**: No compilation required, just a shell script

## Quick Start

### Run in Mock Mode (Recommended for Testing)

```bash
./bitchat-terminal.sh --mock
```

### Run with Verbose Output

```bash
./bitchat-terminal.sh --mock --verbose
```

### Show Help

```bash
./bitchat-terminal.sh --help
```

## Commands

### Basic Commands

- `/help` - Show all available commands
- `/j #channel` - Join or create a channel
- `/m @name message` - Send a private message
- `/w` - List online users
- `/channels` - Show all discovered channels
- `/peers` - List connected peers
- `/nick nickname` - Change your nickname
- `/clear` - Clear chat messages
- `/status` - Show connection status
- `/quit` - Exit application

### Examples

```bash
# Join a channel
/j general

# Send a public message
Hello everyone!

# Send a private message
/m @alice Hello Alice!

# Change nickname
/nick MyName

# Check status
/status

# Exit
/quit
```

## Usage Examples

### Interactive Mode

```bash
$ ./bitchat-terminal.sh --mock
╔══════════════════════════════════════════════════════════════╗
║                    BitChat Terminal                          ║
║              Decentralized Mesh Messaging                   ║
║                    Over Bluetooth LE                        ║
╚══════════════════════════════════════════════════════════════╝

Running in mock mode (no Bluetooth functionality)
Type '/help' for commands or press Ctrl+C to exit.

> /j general
Joined channel: #general
> Hello world!
[User123] Hello world!
> /status
Connection Status:
  Nickname: User123
  Mode: Mock (no Bluetooth)
  Peers connected: 0
  Channels joined: 1
  Status: Running
> /quit
Goodbye!
```

### Script Mode

You can also run commands from a file:

```bash
# Create a command file
echo -e "/j general\nHello world!\n/status\n/quit" > commands.txt

# Run with input file
./bitchat-terminal.sh --mock < commands.txt
```

## Options

- `--mock, -m` - Run in mock mode (no Bluetooth functionality)
- `--verbose, -v` - Enable verbose output
- `--help, -h` - Show help information

## Installation

1. Make the script executable:
   ```bash
   chmod +x bitchat-terminal.sh
   ```

2. Run the application:
   ```bash
   ./bitchat-terminal.sh --mock
   ```

## Requirements

- Bash shell (version 4.0 or higher)
- Unix-like operating system (Linux, macOS, BSD)
- No additional dependencies required

## Mock Mode

The terminal version includes a mock mode that simulates the chat functionality without requiring Bluetooth hardware or special permissions. This is perfect for:

- Testing the interface
- Development and debugging
- Systems without Bluetooth support
- Containerized environments

## Future Enhancements

- Real Bluetooth integration
- Message persistence
- Encryption support
- Network discovery
- File sharing

## Contributing

To extend the terminal version:

1. Edit `bitchat-terminal.sh`
2. Add new commands in the `process_command()` function
3. Test with mock mode
4. Submit pull request

## License

This project is released into the public domain. See the [LICENSE](LICENSE) file for details. 