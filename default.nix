{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation rec {
  pname = "bitchat-linux";
  version = "1.0.0";
  
  src = ./bitchat-linux;
  
  nativeBuildInputs = with pkgs; [
    cmake
    pkg-config
    qt6.wrapQtAppsHook
    makeWrapper
  ];
  
  buildInputs = with pkgs; [
    # Qt6 dependencies
    qt6.qtbase
    qt6.qtconnectivity
    
    # System libraries
    bluez
    libsodium
    lz4
    libnotify
    libcap
    openssl
    
    # Runtime dependencies
    systemd
    dbus
    glib
    zlib
  ];
  
  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
  ];
  
  # Verify source structure and dependencies
  postPatch = ''
    echo "🔍 Verifying source structure..."
    ls -la
    
    # Verify required files exist
    required_files=("CMakeLists.txt" "main.cpp" "BitchatApplication.cpp" "BitchatApplication.h")
    for file in "''${required_files[@]}"; do
      if [ ! -f "$file" ]; then
        echo "❌ ERROR: Required file $file not found"
        exit 1
      fi
    done
    
    # Verify required directories exist
    required_dirs=("Services" "Views" "ViewModels" "Utils" "Protocols")
    for dir in "''${required_dirs[@]}"; do
      if [ ! -d "$dir" ]; then
        echo "❌ ERROR: Required directory $dir not found"
        exit 1
      fi
    done
    
    echo "✅ Source structure verified successfully"
  '';
  
  # Enhanced build phase with better error handling
  buildPhase = ''
    runHook preBuild
    
    echo "🔨 Building BitChat Linux..."
    mkdir -p build
    cd build
    
    # Configure with CMake
    echo "📋 Configuring build with CMake..."
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=$out \
      -DCMAKE_VERBOSE_MAKEFILE=ON
    
    if [ $? -ne 0 ]; then
      echo "❌ ERROR: CMake configuration failed"
      exit 1
    fi
    
    # Build the project
    echo "🏗️ Compiling project..."
    make -j$NIX_BUILD_CORES VERBOSE=1
    
    if [ $? -ne 0 ]; then
      echo "❌ ERROR: Compilation failed"
      exit 1
    fi
    
    # Verify the executable was built
    if [ ! -f bitchat-linux ]; then
      echo "❌ ERROR: bitchat-linux executable not found after build"
      exit 1
    fi
    
    # Test the executable can run (basic check)
    if ! ./bitchat-linux --help >/dev/null 2>&1; then
      echo "⚠️  Warning: bitchat-linux executable may have issues (--help failed)"
    fi
    
    echo "✅ Build completed successfully"
    
    runHook postBuild
  '';
  
  # Enhanced install phase with comprehensive installation
  installPhase = ''
    runHook preInstall
    
    cd build
    
    echo "📦 Installing BitChat Linux..."
    
    # Install main executable
    mkdir -p $out/bin
    cp bitchat-linux $out/bin/
    chmod +x $out/bin/bitchat-linux
    
    # Install desktop entry
    mkdir -p $out/share/applications
    cat > $out/share/applications/bitchat.desktop << 'EOF'
[Desktop Entry]
Name=BitChat
GenericName=Decentralized Messaging
Comment=Secure mesh messaging over Bluetooth LE
Exec=$out/bin/bitchat-linux
Icon=bitchat
Terminal=false
Type=Application
Categories=Network;InstantMessaging;P2P;
Keywords=chat;messaging;bluetooth;mesh;decentralized;
StartupNotify=true
StartupWMClass=bitchat-linux
MimeType=x-scheme-handler/bitchat;
EOF
    
    # Install icon (with fallback)
    mkdir -p $out/share/icons/hicolor/256x256/apps
    if [ -f ../Assets/bitchat.png ]; then
      cp ../Assets/bitchat.png $out/share/icons/hicolor/256x256/apps/
    else
      # Create a professional SVG icon as fallback
      mkdir -p $out/share/pixmaps
      cat > $out/share/pixmaps/bitchat.svg << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <linearGradient id="grad1" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" style="stop-color:#2196F3;stop-opacity:1" />
      <stop offset="100%" style="stop-color:#1976D2;stop-opacity:1" />
    </linearGradient>
  </defs>
  <circle cx="128" cy="128" r="100" fill="url(#grad1)" stroke="#1565C0" stroke-width="4"/>
  <path d="M80 100 L176 100 L176 140 L100 140 L80 160 Z" fill="white" opacity="0.9"/>
  <circle cx="100" cy="120" r="6" fill="#1976D2"/>
  <circle cx="140" cy="120" r="6" fill="#1976D2"/>
  <path d="M90 85 L166 85 M90 155 L166 155" stroke="#1976D2" stroke-width="3" opacity="0.7"/>
  <text x="128" y="200" text-anchor="middle" fill="#1976D2" font-family="Arial, sans-serif" font-size="24" font-weight="bold">BitChat</text>
</svg>
EOF
    fi
    
    # Install documentation
    mkdir -p $out/share/doc/bitchat
    cp ../README.md $out/share/doc/bitchat/ 2>/dev/null || true
    cp ../FEATURE_PARITY.md $out/share/doc/bitchat/ 2>/dev/null || true
    cp ../LICENSE $out/share/doc/bitchat/ 2>/dev/null || true
    
    # Install example configuration
    mkdir -p $out/share/bitchat
    cat > $out/share/bitchat/config.json.example << 'EOF'
{
  "nickname": "BitChatUser",
  "encryption_enabled": true,
  "notifications_enabled": true,
  "dark_mode_enabled": false,
  "battery_optimization": true,
  "bluetooth": {
    "advertisement_interval": 1000,
    "scan_interval": 5000,
    "max_connections": 8,
    "connection_timeout": 30000,
    "adapter": "hci0"
  },
  "channels": {
    "joined": ["general"],
    "passwords": {}
  },
  "ui": {
    "show_system_tray": true,
    "minimize_to_tray": true,
    "close_to_tray": false,
    "theme": "system"
  },
  "logging": {
    "level": "info",
    "file_enabled": false,
    "console_enabled": true,
    "max_file_size": "10MB",
    "max_files": 5
  },
  "security": {
    "auto_accept_connections": false,
    "require_encryption": true,
    "key_rotation_interval": 86400
  }
}
EOF
    
    # Install organized scripts system
    mkdir -p $out/share/bitchat/scripts
    if [ -d ../scripts ]; then
      echo "📋 Installing organized scripts system..."
      cp -r ../scripts/* $out/share/bitchat/scripts/
      chmod +x $out/share/bitchat/scripts/*.sh
      find $out/share/bitchat/scripts -name "*.sh" -type f -exec chmod +x {} \;
    else
      echo "⚠️  Warning: ../scripts directory not found, scripts not installed"
    fi
    
    # Create man page
    mkdir -p $out/share/man/man1
    cat > $out/share/man/man1/bitchat-linux.1 << 'EOF'
.TH BITCHAT-LINUX 1 "2024" "BitChat 1.0.0" "User Commands"
.SH NAME
bitchat-linux \- secure decentralized mesh messaging over Bluetooth LE
.SH SYNOPSIS
.B bitchat-linux
[\fIOPTIONS\fR]
.SH DESCRIPTION
BitChat Linux is a decentralized messaging application that uses Bluetooth LE to create mesh networks for secure communication without internet connectivity.
.SH OPTIONS
.TP
\fB\-\-daemon\fR
Run in daemon mode (no GUI)
.TP
\fB\-\-verbose\fR
Enable verbose logging
.TP
\fB\-\-config\fR \fIFILE\fR
Use specified configuration file
.TP
\fB\-\-help\fR
Show help message
.SH AUTHOR
BitChat Development Team
.SH REPORTING BUGS
Report bugs to the BitChat issue tracker.
.SH COPYRIGHT
Copyright \(co 2024 BitChat Development Team.
.br
License: MIT
.SH SEE ALSO
Full documentation at: https://github.com/your-repo/bitchat
EOF
    
    runHook postInstall
  '';
  
  meta = with pkgs.lib; {
    description = "Decentralized mesh messaging over Bluetooth LE";
    homepage = "https://github.com/your-repo/bitchat";
    license = licenses.mit;
    platforms = platforms.linux;
    maintainers = with maintainers; [ ];
  };
} 