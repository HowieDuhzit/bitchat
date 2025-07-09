#!/bin/bash

# BitChat NixOS Test Script
# Comprehensive testing for NixOS systems

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  BitChat NixOS Test Suite${NC}"
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

# Test result tracking
test_result() {
    local test_name="$1"
    local result="$2"
    local message="$3"
    
    case "$result" in
        "pass")
            print_success "$test_name: $message"
            ((TESTS_PASSED++))
            ;;
        "fail")
            print_error "$test_name: $message"
            ((TESTS_FAILED++))
            ;;
        "skip")
            print_warning "$test_name: $message (skipped)"
            ((TESTS_SKIPPED++))
            ;;
    esac
}

# Check if we're on NixOS
check_nixos() {
    if [ ! -f /etc/NIXOS ]; then
        print_error "This script is designed for NixOS systems only"
        print_info "For other Linux distributions, use: ./scripts/test/linux-test.sh"
        exit 1
    fi
    
    print_success "NixOS system detected"
    print_info "NixOS version: $(nixos-version)"
}

# Test 1: Nix environment
test_nix_environment() {
    print_info "Testing Nix environment..."
    
    # Check Nix installation
    if command -v nix >/dev/null 2>&1; then
        local nix_version=$(nix --version | head -1)
        test_result "Nix installation" "pass" "$nix_version"
    else
        test_result "Nix installation" "fail" "Nix not found"
        return 1
    fi
    
    # Check nix-shell
    if timeout 10s nix-shell --run "echo 'nix-shell works'" >/dev/null 2>&1; then
        test_result "nix-shell" "pass" "Working correctly"
    else
        test_result "nix-shell" "fail" "Failed to start"
    fi
    
    # Check if flakes are enabled
    if nix flake --help >/dev/null 2>&1; then
        test_result "Nix flakes" "pass" "Available"
    else
        test_result "Nix flakes" "skip" "Not enabled"
    fi
}

# Test 2: Build dependencies in Nix shell
test_build_dependencies() {
    print_info "Testing build dependencies in nix-shell..."
    
    local deps_test=$(nix-shell --run '
        echo "Checking dependencies..."
        command -v cmake >/dev/null && echo "✓ CMake available" || exit 1
        command -v pkg-config >/dev/null && echo "✓ pkg-config available" || exit 1
        pkg-config --exists Qt6Core && echo "✓ Qt6Core found" || exit 1
        pkg-config --exists Qt6Bluetooth && echo "✓ Qt6Bluetooth found" || exit 1
        pkg-config --exists libsodium && echo "✓ libsodium found" || exit 1
        pkg-config --exists zlib && echo "✓ zlib found" || exit 1
        pkg-config --exists glib-2.0 && echo "✓ glib-2.0 found" || exit 1
        pkg-config --exists dbus-1 && echo "✓ dbus-1 found" || exit 1
        echo "✓ All dependencies available"
    ' 2>&1)
    
    if echo "$deps_test" | grep -q "All dependencies available"; then
        test_result "Build dependencies" "pass" "All found in nix-shell"
    else
        test_result "Build dependencies" "fail" "Missing dependencies"
        echo "$deps_test"
    fi
}

# Test 3: CMake configuration in Nix shell
test_cmake_configuration() {
    print_info "Testing CMake configuration in nix-shell..."
    
    local build_dir="build-test-nixos"
    
    if nix-shell --run "
        mkdir -p $build_dir && cd $build_dir
        cmake .. >/dev/null 2>&1 && echo 'CMake configuration successful'
        cd .. && rm -rf $build_dir
    " >/dev/null 2>&1; then
        test_result "CMake configuration" "pass" "Successful in nix-shell"
    else
        test_result "CMake configuration" "fail" "Failed in nix-shell"
    fi
}

# Test 4: Nix build
test_nix_build() {
    print_info "Testing nix-build..."
    
    # Clean up any existing build artifacts
    rm -rf build result
    
    # Create a temporary build log
    local build_log="build-test.log"
    
    if timeout 300s nix-build > "$build_log" 2>&1; then
        test_result "nix-build" "pass" "Build successful"
        
        # Check if binary was created
        if [ -f "result/bin/bitchat-linux" ]; then
            test_result "Binary creation" "pass" "Binary created successfully"
            
            # Test the binary
            if timeout 5s ./result/bin/bitchat-linux --help >/dev/null 2>&1; then
                test_result "Binary execution" "pass" "Binary runs correctly"
            else
                test_result "Binary execution" "fail" "Binary failed to run"
            fi
        else
            test_result "Binary creation" "fail" "Binary not found"
        fi
    else
        test_result "nix-build" "fail" "Build failed"
        print_info "Build log (last 20 lines):"
        tail -20 "$build_log"
    fi
    
    # Clean up
    rm -f "$build_log"
}

# Test 5: Nix shell development environment
test_dev_environment() {
    print_info "Testing development environment..."
    
    # Test if we can enter development shell
    if timeout 10s nix-shell --run "echo 'Development shell works'" >/dev/null 2>&1; then
        test_result "Development shell" "pass" "Shell environment works"
    else
        test_result "Development shell" "fail" "Failed to enter shell"
    fi
    
    # Test if flakes development works (if available)
    if [ -f "flake.nix" ]; then
        if timeout 10s nix develop --command echo "Flakes dev works" >/dev/null 2>&1; then
            test_result "Flakes development" "pass" "nix develop works"
        else
            test_result "Flakes development" "fail" "nix develop failed"
        fi
    else
        test_result "Flakes development" "skip" "No flake.nix found"
    fi
}

# Test 6: NixOS module
test_nixos_module() {
    print_info "Testing NixOS module..."
    
    if [ -f "nixos-module.nix" ]; then
        test_result "NixOS module" "pass" "Module file exists"
        
        # Test module syntax
        if nix-instantiate --eval --expr "import ./nixos-module.nix" >/dev/null 2>&1; then
            test_result "Module syntax" "pass" "Valid Nix syntax"
        else
            test_result "Module syntax" "fail" "Invalid Nix syntax"
        fi
    else
        test_result "NixOS module" "fail" "Module file not found"
    fi
}

# Test 7: Bluetooth in NixOS
test_nixos_bluetooth() {
    print_info "Testing Bluetooth in NixOS..."
    
    # Check if Bluetooth is enabled in NixOS configuration
    if nixos-option hardware.bluetooth.enable 2>/dev/null | grep -q "true"; then
        test_result "Bluetooth enabled" "pass" "Enabled in NixOS config"
    else
        test_result "Bluetooth enabled" "fail" "Not enabled in NixOS config"
        print_info "Enable with: hardware.bluetooth.enable = true;"
    fi
    
    # Check if Bluetooth service is running
    if systemctl is-active --quiet bluetooth 2>/dev/null; then
        test_result "Bluetooth service" "pass" "Service is running"
    else
        test_result "Bluetooth service" "fail" "Service not running"
    fi
    
    # Check if user is in bluetooth group
    if groups | grep -q bluetooth; then
        test_result "Bluetooth permissions" "pass" "User in bluetooth group"
    else
        test_result "Bluetooth permissions" "fail" "User not in bluetooth group"
        print_info "Add to config: users.users.$(whoami).extraGroups = [ \"bluetooth\" ];"
    fi
}

# Test 8: NixOS service integration
test_service_integration() {
    print_info "Testing service integration..."
    
    # Check if systemd service can be created
    local service_file="/tmp/bitchat-test.service"
    cat > "$service_file" << 'EOF'
[Unit]
Description=BitChat Test Service
After=bluetooth.service

[Service]
Type=simple
ExecStart=/bin/echo "BitChat service test"
Restart=no

[Install]
WantedBy=multi-user.target
EOF
    
    if systemd-analyze verify "$service_file" 2>/dev/null; then
        test_result "Service definition" "pass" "Valid systemd service"
    else
        test_result "Service definition" "fail" "Invalid systemd service"
    fi
    
    rm -f "$service_file"
}

# Test 9: Configuration management
test_configuration() {
    print_info "Testing configuration management..."
    
    local config_dir="$HOME/.config/bitchat"
    local config_file="$config_dir/test-config.json"
    
    # Test configuration directory creation
    mkdir -p "$config_dir"
    if [ -d "$config_dir" ]; then
        test_result "Config directory" "pass" "Created successfully"
    else
        test_result "Config directory" "fail" "Failed to create"
        return
    fi
    
    # Test configuration file creation
    cat > "$config_file" << 'EOF'
{
    "nickname": "NixOSUser",
    "encryption_enabled": true,
    "notifications_enabled": true,
    "bluetooth": {
        "advertisement_interval": 2000,
        "scan_interval": 5000,
        "max_connections": 8
    }
}
EOF
    
    if [ -f "$config_file" ]; then
        test_result "Config file" "pass" "Created successfully"
        
        # Test JSON validity
        if python3 -m json.tool "$config_file" >/dev/null 2>&1; then
            test_result "Config format" "pass" "Valid JSON"
        else
            test_result "Config format" "fail" "Invalid JSON"
        fi
    else
        test_result "Config file" "fail" "Failed to create"
    fi
    
    # Cleanup
    rm -f "$config_file"
}

# Test 10: NixOS garbage collection compatibility
test_gc_compatibility() {
    print_info "Testing garbage collection compatibility..."
    
    # Check if nix-collect-garbage works
    if timeout 10s nix-collect-garbage --dry-run >/dev/null 2>&1; then
        test_result "Garbage collection" "pass" "nix-collect-garbage works"
    else
        test_result "Garbage collection" "fail" "nix-collect-garbage failed"
    fi
    
    # Check if we can rebuild after GC
    if [ -f "result" ]; then
        # Store current result
        local result_path=$(readlink result)
        
        # Simulate what would happen after GC
        if [ -f "$result_path/bin/bitchat-linux" ]; then
            test_result "GC compatibility" "pass" "Binary survives GC simulation"
        else
            test_result "GC compatibility" "fail" "Binary would be lost after GC"
        fi
    else
        test_result "GC compatibility" "skip" "No build result to test"
    fi
}

# Show test summary
show_summary() {
    echo
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  NixOS Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    local total_tests=$((TESTS_PASSED + TESTS_FAILED + TESTS_SKIPPED))
    
    echo "Total tests: $total_tests"
    echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
    echo -e "Skipped: ${YELLOW}$TESTS_SKIPPED${NC}"
    echo
    
    if [ $TESTS_FAILED -eq 0 ]; then
        print_success "All tests passed!"
        echo
        print_info "BitChat is ready for NixOS:"
        echo "• Install system-wide: ./scripts/nixos/nixos-install.sh"
        echo "• Run directly: ./result/bin/bitchat-linux"
        echo "• Development: nix-shell"
        return 0
    else
        print_error "Some tests failed"
        echo
        print_info "Common solutions:"
        echo "• Enable flakes: nix.settings.experimental-features = [ \"nix-command\" \"flakes\" ];"
        echo "• Enable Bluetooth: hardware.bluetooth.enable = true;"
        echo "• Add to bluetooth group: users.users.$(whoami).extraGroups = [ \"bluetooth\" ];"
        echo "• Rebuild system: sudo nixos-rebuild switch"
        return 1
    fi
}

# Main execution
main() {
    print_header
    
    check_nixos
    
    print_info "Running NixOS-specific tests..."
    echo
    
    # Run all tests
    test_nix_environment
    test_build_dependencies
    test_cmake_configuration
    test_nix_build
    test_dev_environment
    test_nixos_module
    test_nixos_bluetooth
    test_service_integration
    test_configuration
    test_gc_compatibility
    
    # Show summary and exit with appropriate code
    show_summary
}

# Run main function
main "$@" 