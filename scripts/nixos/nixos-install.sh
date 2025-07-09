#!/bin/bash

# BitChat NixOS Installation Script
# Complete setup for BitChat on NixOS

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  BitChat NixOS Installation Script${NC}"
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

# Check if we're on NixOS
check_nixos() {
    if [ ! -f /etc/NIXOS ]; then
        print_error "This script is designed for NixOS systems only"
        print_info "For other Linux distributions, use: ./scripts/build/linux-build.sh"
        exit 1
    fi
    
    print_success "NixOS system detected"
    print_info "NixOS version: $(nixos-version)"
}

# Check for required tools
check_tools() {
    print_info "Checking required tools..."
    
    if ! command -v nix >/dev/null 2>&1; then
        print_error "Nix is not available"
        exit 1
    fi
    
    if ! command -v git >/dev/null 2>&1; then
        print_info "Installing git..."
        nix-env -iA nixos.git
    fi
    
    print_success "All required tools available"
}

# Show installation options
show_options() {
    echo "Installation Options:"
    echo "1) System service (recommended for servers)"
    echo "2) User installation (recommended for desktops)"
    echo "3) Development setup (for contributors)"
    echo "4) Quick test (temporary installation)"
    echo
}

# Get user input
prompt_user() {
    local prompt="$1"
    local default="$2"
    local response
    
    if [ -n "$default" ]; then
        read -p "$prompt [$default]: " response
        echo "${response:-$default}"
    else
        read -p "$prompt: " response
        echo "$response"
    fi
}

# System service installation
install_system_service() {
    print_info "Setting up system service installation..."
    
    local nickname=$(prompt_user "Your nickname" "$(whoami)")
    local enable_verbose=$(prompt_user "Enable verbose logging? (y/n)" "y")
    local enable_notifications=$(prompt_user "Enable notifications? (y/n)" "y")
    
    # Convert y/n to boolean
    [ "$enable_verbose" = "y" ] && enable_verbose="true" || enable_verbose="false"
    [ "$enable_notifications" = "y" ] && enable_notifications="true" || enable_notifications="false"
    
    # Create NixOS configuration
    local config_file="/tmp/bitchat-nixos-config.nix"
    cat > "$config_file" << EOF
# BitChat NixOS Configuration
{
  imports = [
    ./bitchat-linux/nixos-module.nix
  ];

  services.bitchat = {
    enable = true;
    verbose = $enable_verbose;
    enableNotifications = $enable_notifications;
    
    extraConfig = {
      nickname = "$nickname";
      bluetooth = {
        max_connections = 10;
        advertisement_interval = 1000;
        scan_interval = 3000;
      };
      channels = {
        joined = ["general"];
      };
      logging = {
        level = "info";
        file_enabled = true;
        console_enabled = $enable_verbose;
      };
    };
  };

  # Enable Bluetooth
  hardware.bluetooth.enable = true;
  services.blueman.enable = true;
}
EOF
    
    print_success "Configuration created at $config_file"
    print_info "Add this configuration to your /etc/nixos/configuration.nix"
    print_info "Then run: sudo nixos-rebuild switch"
}

# User installation
install_user() {
    print_info "Setting up user installation..."
    
    local repo_dir="$HOME/bitchat-linux"
    
    # Clone or update repository
    if [ -d "$repo_dir" ]; then
        print_info "Updating existing repository..."
        cd "$repo_dir"
        git pull origin main
    else
        print_info "Cloning repository..."
        git clone https://github.com/your-repo/bitchat-linux.git "$repo_dir"
        cd "$repo_dir"
    fi
    
    # Build with Nix
    print_info "Building BitChat with Nix..."
    nix-build
    
    # Create user systemd service
    local service_dir="$HOME/.config/systemd/user"
    mkdir -p "$service_dir"
    
    cat > "$service_dir/bitchat.service" << EOF
[Unit]
Description=BitChat Mesh Messenger
After=bluetooth.service

[Service]
Type=simple
ExecStart=$repo_dir/result/bin/bitchat-linux --verbose
Restart=always
RestartSec=5

[Install]
WantedBy=default.target
EOF
    
    # Enable and start service
    systemctl --user daemon-reload
    systemctl --user enable bitchat.service
    systemctl --user start bitchat.service
    
    print_success "User installation completed"
    print_info "Check status with: systemctl --user status bitchat"
}

# Development setup
install_development() {
    print_info "Setting up development environment..."
    
    local repo_dir="$HOME/bitchat-dev"
    
    # Clone repository
    if [ -d "$repo_dir" ]; then
        print_info "Updating existing repository..."
        cd "$repo_dir"
        git pull origin main
    else
        print_info "Cloning repository..."
        git clone https://github.com/your-repo/bitchat-linux.git "$repo_dir"
        cd "$repo_dir"
    fi
    
    # Enter development shell
    print_info "Development environment ready"
    print_info "To start developing:"
    echo "  cd $repo_dir"
    echo "  nix-shell"
    echo "  # or with flakes:"
    echo "  nix develop"
    echo
    print_info "To build:"
    echo "  nix-build"
    echo "  # or traditional build:"
    echo "  mkdir build && cd build"
    echo "  cmake .. && make"
}

# Quick test installation
install_test() {
    print_info "Setting up quick test installation..."
    
    local temp_dir=$(mktemp -d)
    cd "$temp_dir"
    
    # Clone repository
    git clone https://github.com/your-repo/bitchat-linux.git
    cd bitchat-linux
    
    # Build and test
    nix-build
    
    print_success "Quick test installation completed"
    print_info "Test BitChat with:"
    echo "  $temp_dir/bitchat-linux/result/bin/bitchat-linux --help"
    echo
    print_warning "This is a temporary installation in $temp_dir"
}

# Show post-installation info
show_post_install() {
    print_info "Post-Installation Setup:"
    echo
    echo "1. Enable Bluetooth (if not already enabled):"
    echo "   hardware.bluetooth.enable = true;"
    echo "   services.blueman.enable = true;"
    echo
    echo "2. Add your user to the bluetooth group:"
    echo "   users.users.yourusername.extraGroups = [ \"bluetooth\" ];"
    echo
    echo "3. Rebuild your system:"
    echo "   sudo nixos-rebuild switch"
    echo
    echo "4. Start Bluetooth service:"
    echo "   sudo systemctl start bluetooth"
    echo
    print_info "Common Commands:"
    echo "• Check service status: systemctl status bitchat"
    echo "• View logs: journalctl -u bitchat -f"
    echo "• Test installation: ./scripts/test/nixos-test.sh"
    echo
}

# Main execution
main() {
    print_header
    
    check_nixos
    check_tools
    
    show_options
    
    local choice=$(prompt_user "Select installation method (1-4)" "1")
    
    case "$choice" in
        1)
            install_system_service
            ;;
        2)
            install_user
            ;;
        3)
            install_development
            ;;
        4)
            install_test
            ;;
        *)
            print_error "Invalid selection"
            exit 1
            ;;
    esac
    
    show_post_install
    
    print_success "BitChat NixOS installation completed!"
}

# Run main function
main "$@" 