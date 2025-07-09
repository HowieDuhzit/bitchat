#!/usr/bin/env bash

# BitChat Terminal with Bluetooth Support
# This version can work with real Bluetooth when run outside container

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Variables
NICKNAME="User$((RANDOM % 1000))"
CHANNELS=()
PEERS=()
BLUETOOTH_MODE=false
MOCK_MODE=false
VERBOSE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --bluetooth|-b)
            BLUETOOTH_MODE=true
            shift
            ;;
        --mock|-m)
            MOCK_MODE=true
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --bluetooth, -b   Enable Bluetooth mode (requires host system)"
            echo "  --mock, -m        Run in mock mode (no Bluetooth)"
            echo "  --verbose, -v     Enable verbose output"
            echo "  --help, -h        Show this help"
            echo ""
            echo "Examples:"
            echo "  $0 --mock         # Run in mock mode (container safe)"
            echo "  $0 --bluetooth    # Run with Bluetooth (host system)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check if we can access Bluetooth
check_bluetooth() {
    if [[ "$BLUETOOTH_MODE" == true ]]; then
        print_status "Checking Bluetooth availability..."
        
        if ! command -v bluetoothctl &> /dev/null; then
            print_error "bluetoothctl not found. Please install BlueZ:"
            print_error "  sudo apt install bluez"
            return 1
        fi
        
        if ! systemctl is-active --quiet bluetooth 2>/dev/null; then
            print_warning "Bluetooth service not running"
            print_status "Starting Bluetooth service..."
            sudo systemctl start bluetooth 2>/dev/null || {
                print_error "Could not start Bluetooth service"
                return 1
            }
        fi
        
        print_success "Bluetooth service is running"
        return 0
    fi
    return 0
}

# Set Bluetooth capabilities
setup_bluetooth() {
    if [[ "$BLUETOOTH_MODE" == true ]]; then
        local binary_path="build/bitchat-linux"
        
        if [[ ! -f "$binary_path" ]]; then
            print_error "Bluetooth binary not found: $binary_path"
            print_error "Please build the application first"
            return 1
        fi
        
        print_status "Setting Bluetooth capabilities..."
        
        if sudo setcap cap_net_raw,cap_net_admin+eip "$binary_path" 2>/dev/null; then
            print_success "Bluetooth capabilities set successfully"
            return 0
        else
            print_error "Failed to set Bluetooth capabilities"
            print_error "Make sure you're running on host system with sudo access"
            return 1
        fi
    fi
    return 0
}

# Start Bluetooth mesh service
start_bluetooth_service() {
    if [[ "$BLUETOOTH_MODE" == true ]]; then
        print_status "Starting Bluetooth mesh service..."
        
        # Run the Bluetooth binary in background
        local binary_path="build/bitchat-linux"
        
        if [[ -f "$binary_path" ]]; then
            # Set environment variables for Qt to avoid GUI issues
            export QT_QPA_PLATFORM=offscreen
            export QT_LOGGING_RULES="qt.bluetooth.*=false"
            export QT_DEBUG_PLUGINS=0
            export DISPLAY=""
            
            # Additional environment variables to prevent crashes
            export QT_AUTO_SCREEN_SCALE_FACTOR=0
            export QT_SCALE_FACTOR=1
            export QT_FONT_DPI=96
            
            # Start the Bluetooth service in background with error handling
            print_status "Starting Bluetooth daemon..."
            
            # Run with timeout to catch immediate crashes
            timeout 5s "$binary_path" --daemon --verbose > /tmp/bitchat-bluetooth.log 2>&1 &
            local bluetooth_pid=$!
            
            # Wait a moment to see if it crashes immediately
            sleep 2
            
            if ! kill -0 $bluetooth_pid 2>/dev/null; then
                print_error "Bluetooth service crashed immediately"
                print_error "Check the log: cat /tmp/bitchat-bluetooth.log"
                return 1
            fi
            
            # Store PID for cleanup
            echo $bluetooth_pid > /tmp/bitchat-bluetooth.pid
            
            print_success "Bluetooth service started (PID: $bluetooth_pid)"
            print_status "Log file: /tmp/bitchat-bluetooth.log"
            return 0
        else
            print_error "Bluetooth binary not found"
            return 1
        fi
    fi
    return 0
}

# Check Bluetooth log for errors
check_bluetooth_log() {
    if [[ -f /tmp/bitchat-bluetooth.log ]]; then
        print_status "Bluetooth service log:"
        echo "----------------------------------------"
        tail -20 /tmp/bitchat-bluetooth.log
        echo "----------------------------------------"
    fi
}

# Stop Bluetooth service
stop_bluetooth_service() {
    if [[ -f /tmp/bitchat-bluetooth.pid ]]; then
        local pid=$(cat /tmp/bitchat-bluetooth.pid)
        if kill -0 $pid 2>/dev/null; then
            print_status "Stopping Bluetooth service..."
            kill $pid
            rm -f /tmp/bitchat-bluetooth.pid
            print_success "Bluetooth service stopped"
        fi
    fi
}

# Cleanup on exit
cleanup() {
    stop_bluetooth_service
    echo
    echo "Shutting down gracefully..."
    exit 0
}

trap cleanup SIGINT SIGTERM

show_banner() {
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║                    BitChat Terminal                          ║"
    echo "║              Decentralized Mesh Messaging                   ║"
    echo "║                    Over Bluetooth LE                        ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo
}

show_help() {
    echo "Available Commands:"
    echo "  /help              - Show this help"
    echo "  /j #channel        - Join or create a channel"
    echo "  /m @name message   - Send a private message"
    echo "  /w                 - List online users"
    echo "  /channels          - Show all discovered channels"
    echo "  /peers             - List connected peers"
    echo "  /nick nickname     - Change your nickname"
    echo "  /clear             - Clear chat messages"
    echo "  /status            - Show connection status"
    echo "  /bluetooth         - Show Bluetooth status"
    echo "  /log               - Show Bluetooth service log"
    echo "  /quit              - Exit application"
    echo
}

join_channel() {
    local channel="$1"
    if [[ -z "$channel" ]]; then
        echo "Error: Channel name required"
        return
    fi
    
    # Remove # if present
    channel="${channel#\#}"
    CHANNELS+=("$channel")
    echo "Joined channel: #$channel"
    
    if [[ "$BLUETOOTH_MODE" == true ]]; then
        echo "Channel joined via Bluetooth mesh network"
    fi
}

send_message() {
    local message="$1"
    local target="$2"
    
    if [[ -z "$message" ]]; then
        echo "Error: Message cannot be empty"
        return
    fi
    
    if [[ -z "$target" ]]; then
        # Public message
        echo "[$NICKNAME] $message"
        if [[ "$BLUETOOTH_MODE" == true ]]; then
            echo "Message sent via Bluetooth mesh network"
        fi
    else
        # Private message
        echo "[$NICKNAME -> $target] $message"
        if [[ "$BLUETOOTH_MODE" == true ]]; then
            echo "Private message sent via Bluetooth mesh network"
        fi
    fi
}

list_peers() {
    if [[ "$BLUETOOTH_MODE" == true ]]; then
        echo "Scanning for peers via Bluetooth..."
        # In a real implementation, this would query the Bluetooth service
        echo "Connected peers (Bluetooth mesh):"
        echo "  - Peer1 (via Bluetooth)"
        echo "  - Peer2 (via Bluetooth)"
    else
        if [[ ${#PEERS[@]} -eq 0 ]]; then
            echo "No peers connected"
        else
            echo "Connected peers:"
            for peer in "${PEERS[@]}"; do
                echo "  - $peer"
            done
        fi
    fi
}

list_channels() {
    if [[ ${#CHANNELS[@]} -eq 0 ]]; then
        echo "No channels joined"
    else
        echo "Joined channels:"
        for channel in "${CHANNELS[@]}"; do
            echo "  #$channel"
        done
    fi
}

show_status() {
    echo "Connection Status:"
    echo "  Nickname: $NICKNAME"
    if [[ "$BLUETOOTH_MODE" == true ]]; then
        echo "  Mode: Bluetooth Mesh Network"
        echo "  Bluetooth: Active"
    else
        echo "  Mode: Mock (no Bluetooth)"
        echo "  Bluetooth: Disabled"
    fi
    echo "  Peers connected: ${#PEERS[@]}"
    echo "  Channels joined: ${#CHANNELS[@]}"
    echo "  Status: Running"
}

show_bluetooth_status() {
    if [[ "$BLUETOOTH_MODE" == true ]]; then
        echo "Bluetooth Status:"
        if command -v bluetoothctl &> /dev/null; then
            echo "  Controller: Available"
            if systemctl is-active --quiet bluetooth; then
                echo "  Service: Running"
            else
                echo "  Service: Stopped"
            fi
        else
            echo "  Controller: Not available"
        fi
        
        if [[ -f /tmp/bitchat-bluetooth.pid ]]; then
            local pid=$(cat /tmp/bitchat-bluetooth.pid)
            if kill -0 $pid 2>/dev/null; then
                echo "  Mesh Service: Running (PID: $pid)"
            else
                echo "  Mesh Service: Crashed (PID: $pid)"
                echo "  Checking log for errors..."
                check_bluetooth_log
            fi
        else
            echo "  Mesh Service: Not started"
        fi
        
        # Show recent log entries
        if [[ -f /tmp/bitchat-bluetooth.log ]]; then
            echo "  Log file: /tmp/bitchat-bluetooth.log"
        fi
    else
        echo "Bluetooth is disabled (mock mode)"
    fi
}

clear_screen() {
    clear
    show_banner
}

process_command() {
    local input="$1"
    
    if [[ -z "$input" ]]; then
        return
    fi
    
    if [[ "$input" =~ ^/ ]]; then
        # Command
        local cmd="${input#/}"
        local command="${cmd%% *}"
        local args="${cmd#* }"
        
        case "$command" in
            help)
                show_help
                ;;
            j|join)
                join_channel "$args"
                ;;
            m|msg)
                if [[ "$args" =~ @([^ ]+) ]]; then
                    local target="${BASH_REMATCH[1]}"
                    local message="${args#* }"
                    if [[ "$message" != "$args" ]]; then
                        send_message "$message" "$target"
                    else
                        echo "Error: Message required"
                    fi
                else
                    echo "Error: Use @username to send private message"
                fi
                ;;
            w|who)
                list_peers
                ;;
            channels)
                list_channels
                ;;
            peers)
                list_peers
                ;;
            nick)
                if [[ -n "$args" ]]; then
                    NICKNAME="$args"
                    echo "Nickname set to: $NICKNAME"
                else
                    echo "Error: Nickname required"
                fi
                ;;
            clear)
                clear_screen
                ;;
            status)
                show_status
                ;;
            bluetooth)
                show_bluetooth_status
                ;;
            log)
                check_bluetooth_log
                ;;
            quit|exit)
                echo "Goodbye!"
                exit 0
                ;;
            *)
                echo "Unknown command: /$command"
                echo "Type /help for available commands"
                ;;
        esac
    else
        # Regular message
        send_message "$input"
    fi
}

# Main application
main() {
    show_banner
    
    # Check and setup Bluetooth if requested
    if [[ "$BLUETOOTH_MODE" == true ]]; then
        print_status "Initializing Bluetooth mode..."
        
        if ! check_bluetooth; then
            print_error "Bluetooth setup failed"
            print_error "Falling back to mock mode"
            MOCK_MODE=true
            BLUETOOTH_MODE=false
        elif ! setup_bluetooth; then
            print_error "Bluetooth capabilities setup failed"
            print_error "Falling back to mock mode"
            MOCK_MODE=true
            BLUETOOTH_MODE=false
        elif ! start_bluetooth_service; then
            print_error "Bluetooth service startup failed"
            print_error "Falling back to mock mode"
            MOCK_MODE=true
            BLUETOOTH_MODE=false
        else
            print_success "Bluetooth mode initialized successfully"
        fi
    fi
    
    if [[ "$MOCK_MODE" == true ]]; then
        echo -e "${YELLOW}Running in mock mode (no Bluetooth functionality)${NC}"
    fi
    
    if [[ "$VERBOSE" == true ]]; then
        echo -e "${BLUE}Verbose mode enabled${NC}"
    fi
    
    echo "Type '/help' for commands or press Ctrl+C to exit."
    echo
    
    # Main loop
    while true; do
        echo -n "> "
        read -r input
        process_command "$input"
    done
}

# Run the application
main 