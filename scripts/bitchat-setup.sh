#!/bin/bash

# BitChat Setup Script
# Unified interface for all BitChat setup and build scripts

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  BitChat Setup & Build Tool${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

# Detect operating system
detect_os() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ -f /etc/NIXOS ]]; then
        echo "nixos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    else
        echo "unknown"
    fi
}

# Show usage
show_usage() {
    echo "Usage: $0 <command> [options]"
    echo
    echo "Commands:"
    echo "  setup     - Set up development environment"
    echo "  build     - Build the application"
    echo "  test      - Run tests"
    echo "  install   - Install the application"
    echo "  clean     - Clean build artifacts"
    echo "  help      - Show this help message"
    echo
    echo "Platform-specific options:"
    echo "  --ios     - Target iOS/macOS (requires macOS)"
    echo "  --linux   - Target Linux (standard distributions)"
    echo "  --nixos   - Target NixOS"
    echo
    echo "Examples:"
    echo "  $0 setup           # Auto-detect platform and setup"
    echo "  $0 build --linux   # Build for Linux"
    echo "  $0 test --nixos     # Run NixOS tests"
    echo "  $0 install --linux  # Install on Linux"
    echo
}

# Auto-detect platform or use specified
determine_platform() {
    local specified_platform=""
    
    # Check for platform flags
    for arg in "$@"; do
        case "$arg" in
            --ios|--macos)
                specified_platform="macos"
                ;;
            --linux)
                specified_platform="linux"
                ;;
            --nixos)
                specified_platform="nixos"
                ;;
        esac
    done
    
    # Use specified platform or auto-detect
    if [ -n "$specified_platform" ]; then
        echo "$specified_platform"
    else
        detect_os
    fi
}

# Setup command
cmd_setup() {
    local platform=$(determine_platform "$@")
    
    print_info "Setting up BitChat for $platform..."
    
    case "$platform" in
        macos)
            if [ -f "scripts/setup/ios-setup.sh" ]; then
                chmod +x scripts/setup/ios-setup.sh
                ./scripts/setup/ios-setup.sh
            else
                print_error "iOS/macOS setup script not found"
                exit 1
            fi
            ;;
        nixos)
            if [ -f "scripts/nixos/nixos-install.sh" ]; then
                chmod +x scripts/nixos/nixos-install.sh
                ./scripts/nixos/nixos-install.sh
            else
                print_error "NixOS setup script not found"
                exit 1
            fi
            ;;
        linux)
            print_info "Linux setup: Use package manager to install dependencies"
            print_info "Then run: $0 build --linux"
            ;;
        *)
            print_error "Unsupported platform: $platform"
            exit 1
            ;;
    esac
}

# Build command
cmd_build() {
    local platform=$(determine_platform "$@")
    
    print_info "Building BitChat for $platform..."
    
    case "$platform" in
        macos)
            print_info "For iOS/macOS, use Xcode to build the project"
            print_info "Open bitchat.xcodeproj or Package.swift in Xcode"
            ;;
        nixos)
            print_info "Building with Nix..."
            nix-build
            ;;
        linux)
            if [ -f "scripts/build/linux-build.sh" ]; then
                chmod +x scripts/build/linux-build.sh
                ./scripts/build/linux-build.sh "$@"
            else
                print_error "Linux build script not found"
                exit 1
            fi
            ;;
        *)
            print_error "Unsupported platform: $platform"
            exit 1
            ;;
    esac
}

# Test command
cmd_test() {
    local platform=$(determine_platform "$@")
    
    print_info "Running tests for $platform..."
    
    case "$platform" in
        macos)
            print_info "For iOS/macOS, use Xcode to run tests"
            print_info "Press Cmd+U in Xcode to run unit tests"
            ;;
        nixos)
            if [ -f "scripts/nixos/nixos-test.sh" ]; then
                chmod +x scripts/nixos/nixos-test.sh
                ./scripts/nixos/nixos-test.sh
            else
                print_error "NixOS test script not found"
                exit 1
            fi
            ;;
        linux)
            if [ -f "scripts/test/linux-test.sh" ]; then
                chmod +x scripts/test/linux-test.sh
                ./scripts/test/linux-test.sh
            else
                print_error "Linux test script not found"
                exit 1
            fi
            ;;
        *)
            print_error "Unsupported platform: $platform"
            exit 1
            ;;
    esac
}

# Install command
cmd_install() {
    local platform=$(determine_platform "$@")
    
    print_info "Installing BitChat for $platform..."
    
    case "$platform" in
        macos)
            print_info "For iOS/macOS, install through Xcode"
            print_info "Build and run on device, or archive for distribution"
            ;;
        nixos)
            if [ -f "scripts/nixos/nixos-install.sh" ]; then
                chmod +x scripts/nixos/nixos-install.sh
                ./scripts/nixos/nixos-install.sh
            else
                print_error "NixOS install script not found"
                exit 1
            fi
            ;;
        linux)
            print_info "Building and installing for Linux..."
            if [ -f "scripts/build/linux-build.sh" ]; then
                chmod +x scripts/build/linux-build.sh
                ./scripts/build/linux-build.sh --install "$@"
            else
                print_error "Linux build script not found"
                exit 1
            fi
            ;;
        *)
            print_error "Unsupported platform: $platform"
            exit 1
            ;;
    esac
}

# Clean command
cmd_clean() {
    print_info "Cleaning build artifacts..."
    
    # Remove common build directories
    rm -rf build/
    rm -rf bitchat-linux/build/
    rm -rf result
    rm -rf .build/
    
    # Remove temporary files
    rm -f *.log
    rm -f build-*.log
    
    # Remove Xcode derived data (if on macOS)
    if [[ "$OSTYPE" == "darwin"* ]]; then
        rm -rf ~/Library/Developer/Xcode/DerivedData/bitchat-*
    fi
    
    print_success "Build artifacts cleaned"
}

# Show project status
cmd_status() {
    print_info "BitChat Project Status"
    echo
    
    local platform=$(detect_os)
    echo "Detected platform: $platform"
    echo "Current directory: $(pwd)"
    echo
    
    # Check for key files
    echo "Project files:"
    [ -f "README.md" ] && echo "✅ README.md" || echo "❌ README.md"
    [ -f "bitchat-linux/CMakeLists.txt" ] && echo "✅ Linux CMakeLists.txt" || echo "❌ Linux CMakeLists.txt"
    [ -f "project.yml" ] && echo "✅ iOS project.yml" || echo "❌ iOS project.yml"
    [ -f "shell.nix" ] && echo "✅ Nix shell.nix" || echo "❌ Nix shell.nix"
    echo
    
    # Check for build artifacts
    echo "Build artifacts:"
    [ -d "build" ] && echo "✅ build/ directory" || echo "❌ build/ directory"
    [ -d "bitchat-linux/build" ] && echo "✅ bitchat-linux/build/ directory" || echo "❌ bitchat-linux/build/ directory"
    [ -f "result/bin/bitchat-linux" ] && echo "✅ Nix result binary" || echo "❌ Nix result binary"
    echo
    
    # Check for scripts
    echo "Available scripts:"
    [ -f "scripts/setup/ios-setup.sh" ] && echo "✅ iOS setup" || echo "❌ iOS setup"
    [ -f "scripts/build/linux-build.sh" ] && echo "✅ Linux build" || echo "❌ Linux build"
    [ -f "scripts/test/linux-test.sh" ] && echo "✅ Linux test" || echo "❌ Linux test"
    [ -f "scripts/nixos/nixos-install.sh" ] && echo "✅ NixOS install" || echo "❌ NixOS install"
    [ -f "scripts/nixos/nixos-test.sh" ] && echo "✅ NixOS test" || echo "❌ NixOS test"
}

# Main execution
main() {
    if [ $# -eq 0 ]; then
        print_header
        show_usage
        exit 1
    fi
    
    local command="$1"
    shift
    
    case "$command" in
        setup)
            cmd_setup "$@"
            ;;
        build)
            cmd_build "$@"
            ;;
        test)
            cmd_test "$@"
            ;;
        install)
            cmd_install "$@"
            ;;
        clean)
            cmd_clean "$@"
            ;;
        status)
            cmd_status "$@"
            ;;
        help|--help|-h)
            print_header
            show_usage
            ;;
        *)
            print_error "Unknown command: $command"
            show_usage
            exit 1
            ;;
    esac
}

# Run main function
main "$@" 