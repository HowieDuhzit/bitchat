{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # Build tools
    cmake
    pkg-config
    gcc
    gnumake
    makeWrapper
    
    # Qt6 dependencies
    qt6.qtbase
    qt6.qtconnectivity
    qt6.qttools
    qt6.wrapQtAppsHook
    
    # System libraries
    bluez
    libsodium
    lz4
    libnotify
    libcap
    openssl
    glib
    zlib
    
    # Development tools
    gdb
    valgrind
    clang-tools
    strace
    ltrace
    
    # Runtime dependencies
    systemd
    dbus
    
    # Bluetooth tools
    bluez-tools
    
    # Testing and debugging tools
    socat
    netcat
    tcpdump
    wireshark-cli
    
    # Utilities
    util-linux
    procps
    psmisc
  ];
  
  nativeBuildInputs = with pkgs; [
    qt6.wrapQtAppsHook
    makeWrapper
  ];
  
  shellHook = ''
    echo "🚀 BitChat Linux Development Environment (shell.nix)"
    echo "==================================================="
    echo ""
    echo "📦 Quick Start:"
    echo "  ./scripts/bitchat-setup.sh build --nixos   # Build with main launcher"
    echo "  ./scripts/build/linux-build.sh --verbose   # Direct build script"
    echo "  ./scripts/test/linux-test.sh               # Run comprehensive tests"
    echo "  ./scripts/nixos/nixos-install.sh           # Install on NixOS"
    echo ""
    echo "🔧 Manual Development:"
    echo "  cd bitchat-linux/build && cmake .. && make # Traditional build"
    echo "  gdb ./bitchat-linux/build/bitchat-linux    # Debug with GDB"
    echo "  valgrind ./bitchat-linux/build/bitchat-linux # Memory debugging"
    echo "  strace -e trace=bluetooth ./bitchat-linux/build/bitchat-linux # Trace BT calls"
    echo ""
    echo "🎯 NixOS Integration:"
    echo "  nix-build                           # Build package"
    echo "  nixos-rebuild switch               # Apply NixOS module"
    echo "  services.bitchat.enable = true;    # Enable in configuration.nix"
    echo "  ./scripts/nixos/nixos-test.sh      # Test NixOS integration"
    echo ""
    echo "📋 Available Dependencies:"
    echo "  - CMake $(cmake --version | head -1 | cut -d' ' -f3)"
    echo "  - Qt6 $(qmake6 -query QT_VERSION)"
    echo "  - libsodium $(pkg-config --modversion libsodium)"
    echo "  - BlueZ $(pkg-config --modversion bluez)"
    echo "  - LZ4 $(pkg-config --modversion liblz4)"
    echo "  - libnotify $(pkg-config --modversion libnotify)"
    echo "  - OpenSSL $(pkg-config --modversion openssl)"
    echo "  - glib $(pkg-config --modversion glib-2.0)"
    echo "  - zlib $(pkg-config --modversion zlib)"
    echo ""
    echo "🔐 Bluetooth Setup:"
    echo "  sudo systemctl start bluetooth     # Start Bluetooth service"
    echo "  sudo setcap cap_net_raw+eip ./bitchat-linux/build/bitchat-linux # Set capabilities"
    echo "  bluetoothctl                       # Bluetooth CLI"
    echo "  hciconfig hci0 up                  # Enable adapter"
    echo ""
    echo "🧪 Testing:"
    echo "  ./scripts/test/linux-test.sh       # Linux test suite"
    echo "  ./scripts/nixos/nixos-test.sh      # NixOS test suite"
    echo "  ./scripts/bitchat-setup.sh status  # Check project status"
    echo "  ./scripts/bitchat-setup.sh test    # Auto-platform test"
    echo ""
    echo "🐛 Debugging:"
    echo "  journalctl -u bitchat -f           # View service logs"
    echo "  dmesg | grep -i bluetooth          # Check kernel messages"
    echo "  systemctl status bluetooth         # Check Bluetooth service"
    echo ""
    
    # Set up Qt environment
    export QT_QPA_PLATFORM_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
    export QML2_IMPORT_PATH="${pkgs.qt6.qtbase}/lib/qt-6/qml"
    export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
    
    # Set up pkg-config paths
    export PKG_CONFIG_PATH="${pkgs.bluez}/lib/pkgconfig:${pkgs.libsodium}/lib/pkgconfig:${pkgs.lz4}/lib/pkgconfig:${pkgs.libnotify}/lib/pkgconfig:${pkgs.openssl}/lib/pkgconfig:${pkgs.glib}/lib/pkgconfig:${pkgs.zlib}/lib/pkgconfig:$PKG_CONFIG_PATH"
    
    # Set up library paths
    export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath [
      pkgs.qt6.qtbase
      pkgs.qt6.qtconnectivity
      pkgs.bluez
      pkgs.libsodium
      pkgs.lz4
      pkgs.libnotify
      pkgs.libcap
      pkgs.openssl
      pkgs.glib
      pkgs.zlib
    ]}:$LD_LIBRARY_PATH"
    
    # Create build directory
    mkdir -p bitchat-linux/build
    
    # Check system status
    if [ -f /etc/NIXOS ]; then
      echo "📋 NixOS System Detected"
      echo "  Version: $(nixos-version)"
      echo "  Use: ./scripts/nixos/nixos-install.sh"
      echo "  Config: Add services.bitchat.enable = true;"
      echo ""
    else
      echo "📋 Non-NixOS Linux System"
      echo "  Use: ./scripts/build/linux-build.sh"
      echo "  Package build: nix-build"
      echo ""
    fi
    
    # Check Bluetooth status
    if command -v bluetoothctl >/dev/null 2>&1; then
      if systemctl is-active --quiet bluetooth 2>/dev/null; then
        echo "✅ Bluetooth service is running"
      else
        echo "⚠️  Bluetooth service not running"
        echo "   Start with: sudo systemctl start bluetooth"
      fi
    else
      echo "⚠️  Bluetooth tools not found"
      echo "   Install with: sudo apt install bluez-tools (Ubuntu/Debian)"
    fi
    
    # Check for Bluetooth adapter
    if command -v hciconfig >/dev/null 2>&1; then
      if hciconfig hci0 >/dev/null 2>&1; then
        echo "✅ Bluetooth adapter hci0 detected"
        adapter_info=$(hciconfig hci0 | grep -o "UP RUNNING" || echo "DOWN")
        echo "   Status: $adapter_info"
      else
        echo "⚠️  Bluetooth adapter hci0 not found"
        echo "   Check available adapters: hciconfig -a"
      fi
    fi
    
    # Check scripts directory
    if [ -d "./scripts" ]; then
      echo "✅ Organized scripts system available"
      echo "   Main launcher: ./scripts/bitchat-setup.sh"
      echo "   Build script: ./scripts/build/linux-build.sh"
      echo "   Test script: ./scripts/test/linux-test.sh"
      if [ -f /etc/NIXOS ]; then
        echo "   NixOS install: ./scripts/nixos/nixos-install.sh"
        echo "   NixOS test: ./scripts/nixos/nixos-test.sh"
      fi
    else
      echo "⚠️  Scripts directory not found"
      echo "   Expected: ./scripts/"
    fi
    
    echo ""
    echo "🎉 Environment ready! Use the organized scripts in ./scripts/"
    echo "   Recommended: ./scripts/bitchat-setup.sh build --nixos"
    echo "   For help: ./scripts/bitchat-setup.sh --help"
    echo "   Status check: ./scripts/bitchat-setup.sh status"
  '';
  
  # Set environment variables for CMake to find dependencies
  CMAKE_PREFIX_PATH = with pkgs; lib.makeSearchPathOutput "dev" "lib/cmake" [
    qt6.qtbase
    qt6.qtconnectivity
  ];
  
  # Additional environment variables for development
  BITCHAT_DEV_MODE = "1";
  BITCHAT_LOG_LEVEL = "debug";
  QT_LOGGING_RULES = "*.debug=true";
  
  # Scripts integration
  BITCHAT_SCRIPTS_DIR = "./scripts";
  BITCHAT_BUILD_SYSTEM = "nix";
} 