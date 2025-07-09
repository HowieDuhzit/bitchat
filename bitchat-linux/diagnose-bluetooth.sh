#!/usr/bin/env bash

# BitChat Bluetooth Diagnostic Script
# Helps troubleshoot Bluetooth binary issues

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

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                BitChat Bluetooth Diagnostics                 ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo

# Check binary existence
print_status "Checking binary..."
if [[ -f "build/bitchat-linux" ]]; then
    print_success "Binary found: build/bitchat-linux"
    ls -la build/bitchat-linux
else
    print_error "Binary not found: build/bitchat-linux"
    exit 1
fi

# Check binary type
print_status "Checking binary type..."
if command -v file &> /dev/null; then
    file build/bitchat-linux
else
    print_warning "file command not available"
fi

# Check dependencies
print_status "Checking dependencies..."
if command -v ldd &> /dev/null; then
    ldd build/bitchat-linux 2>/dev/null || {
        print_warning "Could not check dependencies with ldd"
    }
else
    print_warning "ldd command not available"
fi

# Check capabilities
print_status "Checking capabilities..."
if command -v getcap &> /dev/null; then
    getcap build/bitchat-linux 2>/dev/null || {
        print_warning "No capabilities set"
    }
else
    print_warning "getcap command not available"
fi

# Check Bluetooth service
print_status "Checking Bluetooth service..."
if systemctl is-active --quiet bluetooth; then
    print_success "Bluetooth service is running"
else
    print_warning "Bluetooth service is not running"
fi

# Check bluetoothctl
print_status "Checking bluetoothctl..."
if command -v bluetoothctl &> /dev/null; then
    print_success "bluetoothctl is available"
else
    print_error "bluetoothctl not found"
fi

# Test binary with different environments
print_status "Testing binary with different Qt environments..."

# Test 1: Default environment
echo "Test 1: Default environment"
QT_QPA_PLATFORM=offscreen timeout 3s build/bitchat-linux --help > /tmp/test1.log 2>&1 || {
    print_warning "Test 1 failed (default environment)"
    echo "Log:"
    cat /tmp/test1.log
}

# Test 2: Minimal environment
echo "Test 2: Minimal environment"
QT_QPA_PLATFORM=offscreen QT_DEBUG_PLUGINS=0 DISPLAY="" timeout 3s build/bitchat-linux --help > /tmp/test2.log 2>&1 || {
    print_warning "Test 2 failed (minimal environment)"
    echo "Log:"
    cat /tmp/test2.log
}

# Test 3: Daemon mode
echo "Test 3: Daemon mode"
QT_QPA_PLATFORM=offscreen QT_DEBUG_PLUGINS=0 DISPLAY="" timeout 5s build/bitchat-linux --daemon --verbose > /tmp/test3.log 2>&1 &
daemon_pid=$!
sleep 3
if kill -0 $daemon_pid 2>/dev/null; then
    print_success "Daemon mode works"
    kill $daemon_pid
else
    print_warning "Daemon mode failed"
    echo "Log:"
    cat /tmp/test3.log
fi

# Check for Qt plugins
print_status "Checking Qt plugins..."
find /usr/lib -name "*qt*" -type d 2>/dev/null | head -5

# Check system info
print_status "System information..."
echo "OS: $(uname -a)"
echo "Qt version: $(qmake --version 2>/dev/null || echo 'qmake not found')"
echo "Display: $DISPLAY"

echo
echo "Diagnostics complete. Check the logs above for any errors." 