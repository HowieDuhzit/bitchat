#!/bin/bash

# BitChat Linux Test Suite
# Comprehensive testing for Linux systems

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
    echo -e "${BLUE}  BitChat Linux Test Suite${NC}"
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

# Find BitChat executable
find_executable() {
    local executable=""
    
    # Check common locations
    if [ -f "./bitchat-linux" ]; then
        executable="./bitchat-linux"
    elif [ -f "./build/bitchat-linux" ]; then
        executable="./build/bitchat-linux"
    elif [ -f "./bitchat-linux/build/bitchat-linux" ]; then
        executable="./bitchat-linux/build/bitchat-linux"
    elif [ -f "./result/bin/bitchat-linux" ]; then
        executable="./result/bin/bitchat-linux"
    elif command -v bitchat-linux >/dev/null 2>&1; then
        executable="bitchat-linux"
    fi
    
    echo "$executable"
}

# Test 1: Executable existence
test_executable() {
    print_info "Testing executable existence..."
    
    BITCHAT_EXEC=$(find_executable)
    if [ -n "$BITCHAT_EXEC" ]; then
        test_result "Executable" "pass" "Found at $BITCHAT_EXEC"
        
        # Check if it's executable
        if [ -x "$BITCHAT_EXEC" ]; then
            test_result "Permissions" "pass" "Executable permissions set"
        else
            test_result "Permissions" "fail" "Not executable"
        fi
    else
        test_result "Executable" "fail" "Not found"
        print_info "Build the project first:"
        echo "  ./scripts/build/linux-build.sh"
        return 1
    fi
}

# Test 2: Basic functionality
test_basic_functionality() {
    print_info "Testing basic functionality..."
    
    if [ -z "$BITCHAT_EXEC" ]; then
        test_result "Basic functionality" "skip" "No executable found"
        return
    fi
    
    # Test version
    if timeout 5s "$BITCHAT_EXEC" --version >/dev/null 2>&1; then
        test_result "Version check" "pass" "Version command works"
    else
        test_result "Version check" "fail" "Version command failed"
    fi
    
    # Test help
    if timeout 5s "$BITCHAT_EXEC" --help >/dev/null 2>&1; then
        test_result "Help check" "pass" "Help command works"
    else
        test_result "Help check" "fail" "Help command failed"
    fi
}

# Test 3: Dependencies
test_dependencies() {
    print_info "Testing dependencies..."
    
    if [ -z "$BITCHAT_EXEC" ]; then
        test_result "Dependencies" "skip" "No executable found"
        return
    fi
    
    if command -v ldd >/dev/null 2>&1; then
        local missing_deps=$(ldd "$BITCHAT_EXEC" | grep "not found" | wc -l)
        if [ "$missing_deps" -eq 0 ]; then
            test_result "Runtime dependencies" "pass" "All libraries found"
        else
            test_result "Runtime dependencies" "fail" "$missing_deps missing libraries"
            ldd "$BITCHAT_EXEC" | grep "not found"
        fi
    else
        test_result "Runtime dependencies" "skip" "ldd not available"
    fi
}

# Test 4: Configuration system
test_configuration() {
    print_info "Testing configuration system..."
    
    local config_dir="$HOME/.config/bitchat"
    local config_file="$config_dir/test-config.json"
    
    # Create test configuration
    mkdir -p "$config_dir"
    cat > "$config_file" << 'EOF'
{
    "nickname": "TestUser",
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
        test_result "Configuration creation" "pass" "Test config created"
        
        # Test config loading (if executable supports it)
        if [ -n "$BITCHAT_EXEC" ]; then
            if timeout 5s "$BITCHAT_EXEC" --config "$config_file" --help >/dev/null 2>&1; then
                test_result "Configuration loading" "pass" "Config loaded successfully"
            else
                test_result "Configuration loading" "fail" "Config loading failed"
            fi
        fi
        
        # Cleanup
        rm -f "$config_file"
    else
        test_result "Configuration creation" "fail" "Failed to create test config"
    fi
}

# Test 5: Bluetooth system
test_bluetooth() {
    print_info "Testing Bluetooth system..."
    
    # Check if Bluetooth service exists
    if systemctl list-unit-files | grep -q bluetooth.service; then
        test_result "Bluetooth service" "pass" "Service available"
        
        # Check if running
        if systemctl is-active --quiet bluetooth 2>/dev/null; then
            test_result "Bluetooth status" "pass" "Service is running"
        else
            test_result "Bluetooth status" "fail" "Service not running"
            print_info "Start with: sudo systemctl start bluetooth"
        fi
    else
        test_result "Bluetooth service" "fail" "Service not available"
    fi
    
    # Check for Bluetooth tools
    if command -v bluetoothctl >/dev/null 2>&1; then
        test_result "Bluetooth tools" "pass" "bluetoothctl available"
        
        # Check for adapter
        if timeout 5s bluetoothctl show >/dev/null 2>&1; then
            test_result "Bluetooth adapter" "pass" "Adapter found"
        else
            test_result "Bluetooth adapter" "fail" "No adapter found"
        fi
    else
        test_result "Bluetooth tools" "fail" "bluetoothctl not available"
    fi
    
    # Check for hciconfig
    if command -v hciconfig >/dev/null 2>&1; then
        if hciconfig hci0 >/dev/null 2>&1; then
            test_result "HCI interface" "pass" "hci0 available"
        else
            test_result "HCI interface" "fail" "hci0 not available"
        fi
    else
        test_result "HCI interface" "skip" "hciconfig not available"
    fi
}

# Test 6: Capabilities
test_capabilities() {
    print_info "Testing capabilities..."
    
    if [ -z "$BITCHAT_EXEC" ]; then
        test_result "Capabilities" "skip" "No executable found"
        return
    fi
    
    if command -v getcap >/dev/null 2>&1; then
        local caps=$(getcap "$BITCHAT_EXEC" 2>/dev/null)
        if [ -n "$caps" ]; then
            test_result "Capabilities" "pass" "$caps"
        else
            test_result "Capabilities" "fail" "No capabilities set"
            print_info "Set with: sudo setcap cap_net_raw+ep $BITCHAT_EXEC"
        fi
    else
        test_result "Capabilities" "skip" "getcap not available"
    fi
}

# Test 7: Qt environment
test_qt_environment() {
    print_info "Testing Qt environment..."
    
    # Check Qt version
    if command -v qmake >/dev/null 2>&1; then
        local qt_version=$(qmake -version | grep "Qt version" | cut -d' ' -f4)
        test_result "Qt version" "pass" "Qt $qt_version"
    else
        test_result "Qt version" "skip" "qmake not available"
    fi
    
    # Check Qt plugins
    local qt_plugin_paths=(
        "/usr/lib/qt6/plugins"
        "/usr/lib/x86_64-linux-gnu/qt6/plugins"
        "/usr/lib64/qt6/plugins"
    )
    
    local plugins_found=false
    for path in "${qt_plugin_paths[@]}"; do
        if [ -d "$path" ]; then
            test_result "Qt plugins" "pass" "Found in $path"
            plugins_found=true
            break
        fi
    done
    
    if [ "$plugins_found" = false ]; then
        if [ -n "$QT_QPA_PLATFORM_PLUGIN_PATH" ]; then
            test_result "Qt plugins" "pass" "Found via QT_QPA_PLATFORM_PLUGIN_PATH"
        else
            test_result "Qt plugins" "fail" "Not found"
        fi
    fi
}

# Test 8: System integration
test_system_integration() {
    print_info "Testing system integration..."
    
    # Check desktop file
    if [ -f "/usr/share/applications/bitchat.desktop" ]; then
        test_result "Desktop file" "pass" "Installed"
    else
        test_result "Desktop file" "fail" "Not installed"
    fi
    
    # Check icon
    if [ -f "/usr/share/icons/hicolor/256x256/apps/bitchat.png" ]; then
        test_result "Icon" "pass" "Installed"
    else
        test_result "Icon" "fail" "Not installed"
    fi
    
    # Check if installed system-wide
    if command -v bitchat-linux >/dev/null 2>&1; then
        test_result "System installation" "pass" "Available in PATH"
    else
        test_result "System installation" "fail" "Not in PATH"
    fi
}

# Test 9: Memory and performance
test_performance() {
    print_info "Testing performance characteristics..."
    
    if [ -z "$BITCHAT_EXEC" ]; then
        test_result "Performance" "skip" "No executable found"
        return
    fi
    
    # Check binary size
    local size=$(du -h "$BITCHAT_EXEC" | cut -f1)
    test_result "Binary size" "pass" "$size"
    
    # Check stripped status
    if command -v file >/dev/null 2>&1; then
        if file "$BITCHAT_EXEC" | grep -q "not stripped"; then
            test_result "Binary stripping" "fail" "Not stripped (debug build)"
        else
            test_result "Binary stripping" "pass" "Stripped (release build)"
        fi
    fi
}

# Test 10: Security features
test_security() {
    print_info "Testing security features..."
    
    if [ -z "$BITCHAT_EXEC" ]; then
        test_result "Security" "skip" "No executable found"
        return
    fi
    
    # Check for security features
    if command -v checksec >/dev/null 2>&1; then
        local security_info=$(checksec --file="$BITCHAT_EXEC" 2>/dev/null | grep -E "(RELRO|Stack|NX|PIE|RPATH|RUNPATH|Symbols|FORTIFY)")
        if [ -n "$security_info" ]; then
            test_result "Security features" "pass" "Security hardening detected"
        else
            test_result "Security features" "fail" "No security hardening detected"
        fi
    else
        test_result "Security features" "skip" "checksec not available"
    fi
}

# Show test summary
show_summary() {
    echo
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Summary${NC}"
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
        print_info "BitChat Linux is ready to use:"
        if [ -n "$BITCHAT_EXEC" ]; then
            echo "  $BITCHAT_EXEC --verbose"
        fi
        return 0
    else
        print_error "Some tests failed"
        echo
        print_info "Common solutions:"
        echo "• Install missing dependencies"
        echo "• Set Bluetooth capabilities"
        echo "• Start Bluetooth service"
        echo "• Check Qt installation"
        return 1
    fi
}

# Main execution
main() {
    print_header
    
    # Detect environment
    if [ -f /etc/NIXOS ]; then
        print_info "Detected NixOS environment"
    else
        print_info "Detected traditional Linux environment"
    fi
    echo
    
    # Run all tests
    test_executable
    test_basic_functionality
    test_dependencies
    test_configuration
    test_bluetooth
    test_capabilities
    test_qt_environment
    test_system_integration
    test_performance
    test_security
    
    # Show summary and exit with appropriate code
    show_summary
}

# Run main function
main "$@" 