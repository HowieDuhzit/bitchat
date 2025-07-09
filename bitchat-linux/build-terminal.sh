#!/usr/bin/env bash

# BitChat Terminal Build Script
# This script builds the terminal-only version of BitChat

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
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

# Parse command line arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
VERBOSE=false
MOCK_MODE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -m|--mock)
            MOCK_MODE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -d, --debug        Build in debug mode"
            echo "  -c, --clean        Clean build directory"
            echo "  -v, --verbose      Verbose output"
            echo "  -m, --mock         Build with mock Bluetooth support"
            echo "  -h, --help         Show this help"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_status "Building BitChat Terminal ($BUILD_TYPE mode)"

# Check for required tools
check_dependencies() {
    print_status "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for cmake
    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    fi
    
    # Check for make
    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    fi
    
    # Check for pkg-config
    if ! command -v pkg-config &> /dev/null; then
        missing_deps+=("pkg-config")
    fi
    
    # Check for Qt6 Core only
    if ! pkg-config --exists Qt6Core; then
        missing_deps+=("qt6-base-dev")
    fi
    
    # Check for libsodium
    if ! pkg-config --exists libsodium; then
        missing_deps+=("libsodium-dev")
    fi
    
    # Check for zlib
    if ! pkg-config --exists zlib; then
        missing_deps+=("zlib1g-dev")
    fi
    
    # Check for glib
    if ! pkg-config --exists glib-2.0; then
        missing_deps+=("libglib2.0-dev")
    fi
    
    # Check for dbus
    if ! pkg-config --exists dbus-1; then
        missing_deps+=("libdbus-1-dev")
    fi
    
    # Check for libcap
    if ! pkg-config --exists libcap; then
        missing_deps+=("libcap-dev")
    fi
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_error "Please install missing dependencies manually"
        exit 1
    else
        print_success "All dependencies are available"
    fi
}

# Create build directory
setup_build_dir() {
    print_status "Setting up build directory..."
    
    if [[ "$CLEAN_BUILD" == true ]] && [[ -d "build-terminal" ]]; then
        print_status "Cleaning existing build directory..."
        rm -rf build-terminal
    fi
    
    mkdir -p build-terminal
    cd build-terminal
    
    print_success "Build directory ready"
}

# Configure with CMake
configure_cmake() {
    print_status "Configuring build with CMake..."
    
    local cmake_args=(
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DCMAKE_INSTALL_PREFIX="/usr/local"
    )
    
    if [[ "$VERBOSE" == true ]]; then
        cmake_args+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
    fi
    
    if [[ "$MOCK_MODE" == true ]]; then
        cmake_args+=(-DMOCK_BLUETOOTH=ON)
    fi
    
    cmake "${cmake_args[@]}" -f ../CMakeLists-terminal.txt ..
    
    if [[ $? -eq 0 ]]; then
        print_success "CMake configuration completed"
    else
        print_error "CMake configuration failed"
        exit 1
    fi
}

# Build the project
build_project() {
    print_status "Building BitChat Terminal..."
    
    local cpu_count=$(nproc)
    local make_args=("-j$cpu_count")
    
    if [[ "$VERBOSE" == true ]]; then
        make_args+=(VERBOSE=1)
    fi
    
    make "${make_args[@]}"
    
    if [[ $? -eq 0 ]]; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi
}

# Test the build
test_build() {
    print_status "Testing build..."
    
    if [[ -f "./bitchat-terminal" ]]; then
        print_success "Executable created successfully"
        
        # Test help output
        if ./bitchat-terminal --help &> /dev/null; then
            print_success "Application starts correctly"
        else
            print_warning "Application may have issues starting"
        fi
    else
        print_error "Executable not found"
        exit 1
    fi
}

# Main build process
main() {
    check_dependencies
    setup_build_dir
    configure_cmake
    build_project
    test_build
    
    print_success "BitChat Terminal build completed!"
    print_status "You can run the application with:"
    print_status "  ./bitchat-terminal --help"
    print_status "  ./bitchat-terminal --mock (for testing without Bluetooth)"
}

# Run the build
main 