#!/usr/bin/env bash

# BitChat Bluetooth Setup Script
# This script helps set up Bluetooth capabilities for BitChat

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

# Check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_error "This script should not be run as root"
        print_error "Please run as a regular user with sudo access"
        exit 1
    fi
}

# Check Bluetooth availability
check_bluetooth() {
    print_status "Checking Bluetooth availability..."
    
    if ! command -v bluetoothctl &> /dev/null; then
        print_error "bluetoothctl not found. Please install BlueZ:"
        print_error "  sudo apt install bluez"
        return 1
    fi
    
    if ! systemctl is-active --quiet bluetooth; then
        print_warning "Bluetooth service is not running"
        print_status "Starting Bluetooth service..."
        sudo systemctl start bluetooth
        sudo systemctl enable bluetooth
    fi
    
    print_success "Bluetooth service is running"
    return 0
}

# Set capabilities for the binary
set_capabilities() {
    local binary_path="$1"
    
    if [[ ! -f "$binary_path" ]]; then
        print_error "Binary not found: $binary_path"
        return 1
    fi
    
    print_status "Setting Bluetooth capabilities..."
    
    # Set capabilities for raw socket access
    if sudo setcap cap_net_raw,cap_net_admin+eip "$binary_path"; then
        print_success "Capabilities set successfully"
        return 0
    else
        print_error "Failed to set capabilities"
        return 1
    fi
}

# Test Bluetooth functionality
test_bluetooth() {
    print_status "Testing Bluetooth functionality..."
    
    if bluetoothctl show &> /dev/null; then
        print_success "Bluetooth controller is available"
        return 0
    else
        print_error "Bluetooth controller not found"
        return 1
    fi
}

# Run the application
run_application() {
    local binary_path="$1"
    local args="$2"
    
    print_status "Running BitChat with Bluetooth support..."
    print_status "Binary: $binary_path"
    print_status "Args: $args"
    
    if [[ -f "$binary_path" ]]; then
        print_success "Starting BitChat..."
        exec "$binary_path" $args
    else
        print_error "Binary not found: $binary_path"
        exit 1
    fi
}

# Show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --binary PATH    Path to BitChat binary (default: build/bitchat-linux)"
    echo "  --test           Test Bluetooth setup only"
    echo "  --setup          Setup capabilities only"
    echo "  --run [ARGS]     Run the application with optional arguments"
    echo "  --help           Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 --setup                           # Setup capabilities"
    echo "  $0 --test                            # Test Bluetooth"
    echo "  $0 --run --verbose                   # Run with verbose output"
    echo "  $0 --run --mock                      # Run in mock mode"
    echo ""
}

# Main function
main() {
    local binary_path="build/bitchat-linux"
    local action=""
    local run_args=""
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --binary)
                binary_path="$2"
                shift 2
                ;;
            --test)
                action="test"
                shift
                ;;
            --setup)
                action="setup"
                shift
                ;;
            --run)
                action="run"
                shift
                run_args="$@"
                break
                ;;
            --help|-h)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Check if not running as root
    check_root
    
    # Default action if none specified
    if [[ -z "$action" ]]; then
        action="run"
    fi
    
    case "$action" in
        test)
            check_bluetooth
            test_bluetooth
            print_success "Bluetooth setup test completed"
            ;;
        setup)
            check_bluetooth
            set_capabilities "$binary_path"
            print_success "Setup completed"
            ;;
        run)
            check_bluetooth
            set_capabilities "$binary_path"
            run_application "$binary_path" "$run_args"
            ;;
        *)
            print_error "Unknown action: $action"
            exit 1
            ;;
    esac
}

# Run main function
main "$@" 