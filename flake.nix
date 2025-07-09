{
  description = "BitChat - Secure decentralized mesh messaging over Bluetooth LE";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        
        # Import the package from default.nix
        bitchat = pkgs.callPackage ./default.nix {};
        
      in
      {
        packages = {
          default = bitchat;
          bitchat-linux = bitchat;
        };
        
        devShells.default = pkgs.mkShell {
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
            
            # Testing tools
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
            echo "🚀 BitChat Linux Development Environment (Flake)"
            echo "==============================================="
            echo ""
            echo "📦 Quick Commands:"
            echo "  nix build                       # Build package"
            echo "  nix run                         # Run BitChat"
            echo "  nix run .#daemon                # Run as daemon"
            echo "  nix run .#verbose               # Run with verbose output"
            echo "  nix develop                     # Enter this dev shell"
            echo ""
            echo "🔧 Development (Organized Scripts):"
            echo "  ./scripts/bitchat-setup.sh build --nixos       # Main build launcher"
            echo "  ./scripts/build/linux-build.sh --verbose       # Direct build script"
            echo "  ./scripts/test/linux-test.sh                   # Run test suite"
            echo "  ./scripts/setup/system-setup.sh --nixos        # System setup"
            echo ""
            echo "📋 Manual Development:"
            echo "  cd bitchat-linux/build && cmake .. && make     # Traditional build"
            echo "  gdb ./bitchat-linux/build/bitchat-linux        # Debug with GDB"
            echo "  valgrind ./bitchat-linux/build/bitchat-linux   # Memory debugging"
            echo ""
            echo "🎯 NixOS Integration:"
            echo "  services.bitchat.enable = true;    # Enable in configuration.nix"
            echo "  ./scripts/nixos/nixos-install.sh   # Install script"
            echo "  ./scripts/nixos/nixos-test.sh      # Test NixOS setup"
            echo "  nixos-rebuild switch               # Apply system changes"
            echo ""
            echo "📋 Dependencies Available:"
            echo "  - CMake $(cmake --version | head -1 | cut -d' ' -f3)"
            echo "  - Qt6 $(qmake6 -query QT_VERSION)"
            echo "  - libsodium $(pkg-config --modversion libsodium)"
            echo "  - BlueZ $(pkg-config --modversion bluez)"
            echo "  - LZ4 $(pkg-config --modversion liblz4)"
            echo "  - libnotify $(pkg-config --modversion libnotify)"
            echo "  - OpenSSL $(pkg-config --modversion openssl)"
            echo ""
            echo "🔐 Bluetooth Setup:"
            echo "  sudo systemctl start bluetooth"
            echo "  sudo setcap cap_net_raw+eip ./bitchat-linux/build/bitchat-linux"
            echo "  bluetoothctl # Test Bluetooth CLI"
            echo ""
            echo "🧪 Testing:"
            echo "  ./scripts/test/linux-test.sh       # Comprehensive Linux tests"
            echo "  ./scripts/nixos/nixos-test.sh      # NixOS-specific tests"
            echo "  ./scripts/bitchat-setup.sh test    # Platform-auto test"
            echo ""
            echo "🐛 Debugging:"
            echo "  journalctl -u bitchat -f           # View service logs"
            echo "  gdb ./bitchat-linux/build/bitchat-linux  # Debug with GDB"
            echo "  valgrind ./bitchat-linux/build/bitchat-linux  # Memory check"
            echo "  strace -e trace=bluetooth ./bitchat-linux/build/bitchat-linux  # Trace BT calls"
            echo ""
            
            # Set up environment variables
            export QT_QPA_PLATFORM_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
            export QML2_IMPORT_PATH="${pkgs.qt6.qtbase}/lib/qt-6/qml"
            export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
            export PKG_CONFIG_PATH="${pkgs.bluez}/lib/pkgconfig:${pkgs.libsodium}/lib/pkgconfig:${pkgs.lz4}/lib/pkgconfig:${pkgs.libnotify}/lib/pkgconfig:${pkgs.openssl}/lib/pkgconfig:$PKG_CONFIG_PATH"
            
            # Create build directory
            mkdir -p bitchat-linux/build
            
            # Check system status
            if [ -f /etc/NIXOS ]; then
              echo "📋 NixOS System Detected"
              echo "  NixOS version: $(nixos-version)"
              echo "  Add to configuration.nix:"
              echo "    services.bitchat.enable = true;"
              echo "    hardware.bluetooth.enable = true;"
              echo "  Install: ./scripts/nixos/nixos-install.sh"
              echo ""
            else
              echo "📋 Non-NixOS Linux System"
              echo "  Use: ./scripts/build/linux-build.sh"
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
            else
              echo "⚠️  Scripts directory not found"
            fi
            
            echo ""
            echo "🎉 Environment ready! Use the organized scripts in ./scripts/"
            echo "   Recommended: ./scripts/bitchat-setup.sh build --nixos"
            echo "   For help: ./scripts/bitchat-setup.sh --help"
          '';
          
          # Environment variables for development
          CMAKE_PREFIX_PATH = with pkgs; pkgs.lib.makeSearchPathOutput "dev" "lib/cmake" [
            qt6.qtbase
            qt6.qtconnectivity
          ];
          
          LD_LIBRARY_PATH = with pkgs; pkgs.lib.makeLibraryPath [
            qt6.qtbase
            qt6.qtconnectivity
            bluez
            libsodium
            lz4
            libnotify
            libcap
            openssl
            glib
            zlib
          ];
          
          # Development environment flags
          BITCHAT_DEV_MODE = "1";
          BITCHAT_LOG_LEVEL = "debug";
          QT_LOGGING_RULES = "*.debug=true";
          
          # Scripts integration
          BITCHAT_SCRIPTS_DIR = "./scripts";
          BITCHAT_BUILD_SYSTEM = "nix";
        };
        
        # Default app
        apps.default = flake-utils.lib.mkApp {
          drv = bitchat;
          name = "bitchat-linux";
        };
        
        # Daemon mode app
        apps.daemon = flake-utils.lib.mkApp {
          drv = pkgs.writeShellScript "bitchat-daemon" ''
            exec ${bitchat}/bin/bitchat-linux --daemon "$@"
          '';
        };
        
        # Verbose mode app
        apps.verbose = flake-utils.lib.mkApp {
          drv = pkgs.writeShellScript "bitchat-verbose" ''
            exec ${bitchat}/bin/bitchat-linux --verbose "$@"
          '';
        };
        
        # Setup script app
        apps.setup = flake-utils.lib.mkApp {
          drv = pkgs.writeShellScript "bitchat-setup" ''
            cd ${self}
            if [ -f "./scripts/bitchat-setup.sh" ]; then
              exec ./scripts/bitchat-setup.sh "$@"
            else
              echo "❌ ERROR: Organized scripts not found"
              echo "   Expected: ./scripts/bitchat-setup.sh"
              exit 1
            fi
          '';
        };
        
        # Test script app
        apps.test = flake-utils.lib.mkApp {
          drv = pkgs.writeShellScript "bitchat-test" ''
            cd ${self}
            if [ -f /etc/NIXOS ]; then
              if [ -f "./scripts/nixos/nixos-test.sh" ]; then
                exec ./scripts/nixos/nixos-test.sh "$@"
              else
                echo "❌ ERROR: NixOS test script not found"
                exit 1
              fi
            else
              if [ -f "./scripts/test/linux-test.sh" ]; then
                exec ./scripts/test/linux-test.sh "$@"
              else
                echo "❌ ERROR: Linux test script not found"
                exit 1
              fi
            fi
          '';
        };
        
        # Build script app
        apps.build = flake-utils.lib.mkApp {
          drv = pkgs.writeShellScript "bitchat-build" ''
            cd ${self}
            if [ -f "./scripts/build/linux-build.sh" ]; then
              exec ./scripts/build/linux-build.sh "$@"
            else
              echo "❌ ERROR: Build script not found"
              echo "   Expected: ./scripts/build/linux-build.sh"
              exit 1
            fi
          '';
        };
        
        # Status check app
        apps.status = flake-utils.lib.mkApp {
          drv = pkgs.writeShellScript "bitchat-status" ''
            cd ${self}
            if [ -f "./scripts/bitchat-setup.sh" ]; then
              exec ./scripts/bitchat-setup.sh status "$@"
            else
              echo "❌ ERROR: Status script not found"
              exit 1
            fi
          '';
        };
      }
    ) // {
      # NixOS module
      nixosModules.default = import ./nixos-module.nix;
      nixosModules.bitchat = import ./nixos-module.nix;
      
      # Overlay for adding to nixpkgs
      overlays.default = final: prev: {
        bitchat-linux = final.callPackage ./default.nix {};
      };
      
      # Templates for new projects
      templates.default = {
        path = ./.;
        description = "BitChat Linux project template";
        welcomeText = ''
          # BitChat Linux Development Template
          
          This template provides a complete development environment for BitChat Linux.
          
          ## Quick Start
          
          1. Enter the development shell:
             ```bash
             nix develop
             ```
          
          2. Build the project:
             ```bash
             ./scripts/bitchat-setup.sh build --nixos
             ```
          
          3. Run tests:
             ```bash
             ./scripts/test/linux-test.sh
             ```
          
          ## Available Commands
          
          - `nix build` - Build the package
          - `nix run` - Run BitChat
          - `nix run .#daemon` - Run as daemon
          - `nix run .#test` - Run tests
          - `nix run .#setup` - Run setup script
          
          ## NixOS Integration
          
          Add to your `configuration.nix`:
          ```nix
          services.bitchat.enable = true;
          hardware.bluetooth.enable = true;
          ```
          
          Then run:
          ```bash
          ./scripts/nixos/nixos-install.sh
          ```
        '';
      };
    };
} 