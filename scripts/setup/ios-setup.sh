#!/bin/bash

# BitChat iOS/macOS Setup Script
# Sets up the iOS/macOS development environment

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  BitChat iOS/macOS Setup Script${NC}"
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

# Check if we're in the right directory
check_directory() {
    if [ ! -f "project.yml" ] && [ ! -f "Package.swift" ]; then
        print_error "This script must be run from the BitChat root directory"
        print_info "Expected files: project.yml or Package.swift"
        exit 1
    fi
}

# Check for Xcode
check_xcode() {
    if ! command -v xcodebuild &> /dev/null; then
        print_error "Xcode is not installed or not in PATH"
        print_info "Please install Xcode from the App Store"
        exit 1
    fi
    
    print_success "Xcode found: $(xcodebuild -version | head -1)"
}

# Setup using XcodeGen
setup_xcodegen() {
    print_info "Setting up project with XcodeGen..."
    
    if command -v xcodegen &> /dev/null; then
        print_success "XcodeGen found"
        
        if [ -f "project.yml" ]; then
            print_info "Generating Xcode project..."
            xcodegen generate
            print_success "Project generated successfully"
            
            print_info "To open the project:"
            echo "  open bitchat.xcodeproj"
        else
            print_warning "project.yml not found"
        fi
    else
        print_warning "XcodeGen not found"
        print_info "Install with: brew install xcodegen"
        return 1
    fi
}

# Setup using Swift Package Manager
setup_spm() {
    print_info "Setting up project with Swift Package Manager..."
    
    if [ -f "Package.swift" ]; then
        print_success "Package.swift found"
        print_info "To open the project:"
        echo "  open Package.swift"
        echo "  # or"
        echo "  swift package generate-xcodeproj"
    else
        print_warning "Package.swift not found"
        return 1
    fi
}

# Show project structure
show_structure() {
    print_info "Project Structure:"
    echo "📁 bitchat/                   # Main source files"
    echo "  ├── BitchatApp.swift        # App entry point"
    echo "  ├── Views/                  # SwiftUI views"
    echo "  │   ├── ContentView.swift   # Main chat interface"
    echo "  │   ├── ChannelView.swift   # Channel management"
    echo "  │   └── SettingsView.swift  # App settings"
    echo "  ├── ViewModels/             # View models"
    echo "  │   ├── ChatViewModel.swift # Chat state management"
    echo "  │   └── SettingsViewModel.swift # Settings management"
    echo "  ├── Services/               # Core services"
    echo "  │   ├── BluetoothMeshService.swift # Bluetooth networking"
    echo "  │   ├── EncryptionService.swift   # End-to-end encryption"
    echo "  │   └── MessageRetentionService.swift # Message storage"
    echo "  ├── Protocols/              # Protocol definitions"
    echo "  │   ├── BitchatProtocol.swift # Core protocol"
    echo "  │   └── BinaryProtocol.swift  # Binary encoding"
    echo "  └── Utils/                  # Utility classes"
    echo "      ├── OptimizedBloomFilter.swift # Duplicate detection"
    echo "      └── BatteryOptimizer.swift     # Power management"
    echo
}

# Show important notes
show_notes() {
    print_info "Important Notes:"
    echo "• Bluetooth functionality requires physical devices"
    echo "• The iOS Simulator does not support Bluetooth"
    echo "• Test with multiple devices for mesh functionality"
    echo "• Enable Bluetooth in device settings"
    echo "• Grant Bluetooth permissions when prompted"
    echo
    
    print_info "Next Steps:"
    echo "1. Open the project in Xcode"
    echo "2. Select your development team"
    echo "3. Choose a physical device target"
    echo "4. Build and run the app"
    echo "5. Test with multiple devices"
    echo
}

# Main execution
main() {
    print_header
    
    check_directory
    check_xcode
    
    # Try XcodeGen first, then fallback to SPM
    if ! setup_xcodegen; then
        setup_spm
    fi
    
    show_structure
    show_notes
    
    print_success "Setup complete! Ready to build BitChat for iOS/macOS"
}

# Run main function
main "$@" 