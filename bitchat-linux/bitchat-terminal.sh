#!/usr/bin/env bash

# BitChat Terminal - Simple Shell Version
# A basic terminal interface for BitChat

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Variables
NICKNAME="User$((RANDOM % 1000))"
CHANNELS=()
PEERS=()
MOCK_MODE=false
VERBOSE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
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
            echo "  --mock, -m      Run in mock mode (no Bluetooth)"
            echo "  --verbose, -v    Enable verbose output"
            echo "  --help, -h       Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

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
    else
        # Private message
        echo "[$NICKNAME -> $target] $message"
    fi
}

list_peers() {
    if [[ ${#PEERS[@]} -eq 0 ]]; then
        echo "No peers connected"
    else
        echo "Connected peers:"
        for peer in "${PEERS[@]}"; do
            echo "  - $peer"
        done
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
    echo "  Mode: $($MOCK_MODE && echo "Mock (no Bluetooth)" || echo "Normal")"
    echo "  Peers connected: ${#PEERS[@]}"
    echo "  Channels joined: ${#CHANNELS[@]}"
    echo "  Status: Running"
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

# Signal handler
cleanup() {
    echo
    echo "Shutting down gracefully..."
    exit 0
}

trap cleanup SIGINT SIGTERM

# Main application
main() {
    show_banner
    
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