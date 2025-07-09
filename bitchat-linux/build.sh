#!/bin/bash

# BitChat Linux Build Script
# This script builds the complete Linux port of BitChat

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

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   print_error "This script should not be run as root"
   exit 1
fi

# Parse command line arguments
BUILD_TYPE="Release"
INSTALL_DEPS=false
CLEAN_BUILD=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -i|--install-deps)
            INSTALL_DEPS=true
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
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -d, --debug        Build in debug mode"
            echo "  -i, --install-deps Install dependencies"
            echo "  -c, --clean        Clean build directory"
            echo "  -v, --verbose      Verbose output"
            echo "  -h, --help         Show this help"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_status "Building BitChat Linux ($BUILD_TYPE mode)"

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
    
    # Check for Qt6
    if ! pkg-config --exists Qt6Core Qt6Widgets Qt6Bluetooth; then
        missing_deps+=("qt6-base-dev" "qt6-bluetooth-dev")
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
        
        if [[ "$INSTALL_DEPS" == true ]]; then
            print_status "Installing dependencies..."
            
            # Detect package manager
            if command -v apt-get &> /dev/null; then
                sudo apt-get update
                sudo apt-get install -y cmake build-essential pkg-config \
                    qt6-base-dev qt6-bluetooth-dev libsodium-dev zlib1g-dev \
                    libglib2.0-dev libdbus-1-dev libcap-dev
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y cmake gcc-c++ pkg-config \
                    qt6-qtbase-devel qt6-qtconnectivity-devel libsodium-devel \
                    zlib-devel glib2-devel dbus-devel libcap-devel
            elif command -v pacman &> /dev/null; then
                sudo pacman -S --noconfirm cmake gcc pkg-config \
                    qt6-base qt6-connectivity libsodium zlib \
                    glib2 dbus libcap
            else
                print_error "Unsupported package manager. Please install dependencies manually."
                exit 1
            fi
            
            print_success "Dependencies installed successfully"
        else
            print_error "Please install missing dependencies or use --install-deps"
            exit 1
        fi
    else
        print_success "All dependencies are available"
    fi
}

# Create build directory
setup_build_dir() {
    print_status "Setting up build directory..."
    
    if [[ "$CLEAN_BUILD" == true ]] && [[ -d "build" ]]; then
        print_status "Cleaning existing build directory..."
        rm -rf build
    fi
    
    mkdir -p build
    cd build
    
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
    
    cmake "${cmake_args[@]}" ..
    
    if [[ $? -eq 0 ]]; then
        print_success "CMake configuration completed"
    else
        print_error "CMake configuration failed"
        exit 1
    fi
}

# Build the project
build_project() {
    print_status "Building BitChat Linux..."
    
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

# Set up capabilities
setup_capabilities() {
    print_status "Setting up Bluetooth capabilities..."
    
    local executable="./bitchat-linux"
    
    if [[ -f "$executable" ]]; then
        # Set capabilities for raw socket access
        if command -v setcap &> /dev/null; then
            if sudo setcap cap_net_raw+ep "$executable"; then
                print_success "Capabilities set successfully"
            else
                print_warning "Failed to set capabilities. You may need to run as root or with sudo."
            fi
        else
            print_warning "setcap not available. You may need to run as root for Bluetooth access."
        fi
    else
        print_error "Executable not found: $executable"
        exit 1
    fi
}

# Test the build
test_build() {
    print_status "Testing the build..."
    
    local executable="./bitchat-linux"
    
    if [[ -f "$executable" ]]; then
        # Test basic functionality
        if timeout 5s "$executable" --help &> /dev/null; then
            print_success "Basic functionality test passed"
        else
            print_warning "Basic functionality test failed or timed out"
        fi
        
        # Check dependencies
        if ldd "$executable" | grep -q "not found"; then
            print_error "Missing runtime dependencies:"
            ldd "$executable" | grep "not found"
            exit 1
        else
            print_success "All runtime dependencies are available"
        fi
    else
        print_error "Executable not found: $executable"
        exit 1
    fi
}

# Install the application
install_app() {
    print_status "Installing BitChat Linux..."
    
    if make install; then
        print_success "Installation completed"
        
        # Install desktop file and icon
        if [[ -f "../Assets/bitchat.desktop" ]]; then
            sudo cp "../Assets/bitchat.desktop" "/usr/share/applications/"
            print_success "Desktop file installed"
        fi
        
        if [[ -f "../Assets/bitchat.png" ]]; then
            sudo mkdir -p "/usr/share/icons/hicolor/256x256/apps"
            sudo cp "../Assets/bitchat.png" "/usr/share/icons/hicolor/256x256/apps/"
            print_success "Icon installed"
        fi
        
        # Update desktop database
        if command -v update-desktop-database &> /dev/null; then
            sudo update-desktop-database /usr/share/applications
        fi
        
        print_success "BitChat Linux installed successfully"
        print_status "You can now run 'bitchat-linux' from anywhere"
    else
        print_error "Installation failed"
        exit 1
    fi
}

# Main execution
main() {
    print_status "Starting BitChat Linux build process..."
    
    # Check if we're in the right directory
    if [[ ! -f "CMakeLists.txt" ]]; then
        print_error "CMakeLists.txt not found. Please run this script from the bitchat-linux directory."
        exit 1
    fi
    
    check_dependencies
    setup_build_dir
    configure_cmake
    build_project
    setup_capabilities
    test_build
    
    print_success "BitChat Linux build completed successfully!"
    print_status "Executable location: $(pwd)/bitchat-linux"
    
    # Ask if user wants to install
    read -p "Do you want to install BitChat Linux system-wide? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        install_app
    else
        print_status "Skipping installation. You can run ./bitchat-linux from the build directory."
    fi
    
    print_status "Build summary:"
    echo "  - Build type: $BUILD_TYPE"
    echo "  - Executable: $(pwd)/bitchat-linux"
    echo "  - Size: $(du -h bitchat-linux | cut -f1)"
    echo "  - Dependencies: $(ldd bitchat-linux | wc -l) libraries"
    
    print_success "BitChat Linux is ready to use!"
}

# Run main function
main "$@" 