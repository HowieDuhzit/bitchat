#!/usr/bin/env bash

# BitChat Bluetooth Setup Script - Container Version
# This script helps set up Bluetooth capabilities for BitChat in container environments

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

# Check if running in container
check_container() {
    if [[ -f /.dockerenv ]] || [[ -f /run/.containerenv ]] || [[ -n "$container" ]]; then
        print_warning "Running in container environment"
        print_warning "Bluetooth capabilities may be limited"
        return 0
    else
        print_status "Running on host system"
        return 1
    fi
}

# Check Bluetooth availability
check_bluetooth() {
    print_status "Checking Bluetooth availability..."
    
    if ! command -v bluetoothctl &> /dev/null; then
        print_warning "bluetoothctl not found"
        print_status "This is normal in container environments"
        return 1
    fi
    
    if ! systemctl is-active --quiet bluetooth 2>/dev/null; then
        print_warning "Bluetooth service not available"
        print_status "This is normal in container environments"
        return 1
    fi
    
    print_success "Bluetooth service is running"
    return 0
}

# Set capabilities for the binary (container-safe)
set_capabilities() {
    local binary_path="$1"
    
    if [[ ! -f "$binary_path" ]]; then
        print_error "Binary not found: $binary_path"
        return 1
    fi
    
    print_status "Setting Bluetooth capabilities..."
    
    # Try to set capabilities, but don't fail if we can't
    if sudo setcap cap_net_raw,cap_net_admin+eip "$binary_path" 2>/dev/null; then
        print_success "Capabilities set successfully"
        return 0
    else
        print_warning "Could not set capabilities (container restriction)"
        print_warning "Application may not work with Bluetooth"
        return 1
    fi
}

# Test Bluetooth functionality
test_bluetooth() {
    print_status "Testing Bluetooth functionality..."
    
    if command -v bluetoothctl &> /dev/null && bluetoothctl show &> /dev/null; then
        print_success "Bluetooth controller is available"
        return 0
    else
        print_warning "Bluetooth controller not available"
        print_warning "This is normal in container environments"
        return 1
    fi
}

# Run the application with container-safe options
run_application() {
    local binary_path="$1"
    local args="$2"
    
    print_status "Running BitChat with container-safe options..."
    print_status "Binary: $binary_path"
    print_status "Args: $args"
    
    if [[ ! -f "$binary_path" ]]; then
        print_error "Binary not found: $binary_path"
        exit 1
    fi
    
    # Set container-safe environment variables
    export QT_QPA_PLATFORM=offscreen
    export QT_LOGGING_RULES="qt.bluetooth.*=false"
    
    print_success "Starting BitChat..."
    print_warning "Running in container mode - Bluetooth may not work"
    
    exec "$binary_path" $args
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
    echo "  --mock           Run in mock mode (recommended for containers)"
    echo "  --help           Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 --setup                           # Setup capabilities"
    echo "  $0 --test                            # Test Bluetooth"
    echo "  $0 --run --verbose                   # Run with verbose output"
    echo "  $0 --run --mock                      # Run in mock mode"
    echo "  $0 --mock                            # Run terminal version in mock mode"
    echo ""
}

# Main function
main() {
    local binary_path="build/bitchat-linux"
    local action=""
    local run_args=""
    local mock_mode=false
    
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
            --mock)
                mock_mode=true
                action="mock"
                shift
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
    
    # Check if we're in a container
    check_container
    
    # Default action if none specified
    if [[ -z "$action" ]]; then
        if [[ "$mock_mode" == true ]]; then
            action="mock"
        else
            action="run"
        fi
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
        mock)
            print_status "Running terminal version in mock mode..."
            exec ./bitchat-terminal.sh --mock
            ;;
        *)
            print_error "Unknown action: $action"
            exit 1
            ;;
    esac
}

# Run main function
main "$@" 