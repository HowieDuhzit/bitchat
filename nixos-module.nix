{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.bitchat;
  bitchat = pkgs.callPackage ./default.nix {};
  
  # Default configuration
  defaultConfig = {
    nickname = "BitChatNode";
    encryption_enabled = true;
    notifications_enabled = false;  # Disabled for daemon mode
    dark_mode_enabled = false;
    battery_optimization = true;
    bluetooth = {
      advertisement_interval = 2000;
      scan_interval = 5000;
      max_connections = 8;
      connection_timeout = 30000;
      adapter = "hci0";
    };
    channels = {
      joined = ["general"];
      passwords = {};
    };
    ui = {
      show_system_tray = false;
      minimize_to_tray = false;
      close_to_tray = false;
      theme = "system";
    };
    logging = {
      level = "info";
      file_enabled = true;
      console_enabled = false;
      max_file_size = "10MB";
      max_files = 5;
    };
    security = {
      auto_accept_connections = false;
      require_encryption = true;
      key_rotation_interval = 86400;
    };
  };
  
  configFile = pkgs.writeText "bitchat-config.json" 
    (builtins.toJSON (defaultConfig // cfg.extraConfig));
in

{
  options.services.bitchat = {
    enable = mkEnableOption "BitChat decentralized messaging service";
    
    package = mkOption {
      type = types.package;
      default = bitchat;
      description = "The BitChat package to use";
    };
    
    user = mkOption {
      type = types.str;
      default = "bitchat";
      description = "User to run BitChat daemon as";
    };
    
    group = mkOption {
      type = types.str;
      default = "bitchat";
      description = "Group to run BitChat daemon as";
    };
    
    dataDir = mkOption {
      type = types.str;
      default = "/var/lib/bitchat";
      description = "Directory to store BitChat data";
    };
    
    configFile = mkOption {
      type = types.nullOr types.path;
      default = null;
      description = "Path to BitChat configuration file (overrides extraConfig)";
    };
    
    extraConfig = mkOption {
      type = types.attrs;
      default = {};
      description = "Additional configuration options";
      example = {
        nickname = "MyNode";
        bluetooth.max_connections = 10;
        channels.joined = ["general" "tech"];
      };
    };
    
    verbose = mkOption {
      type = types.bool;
      default = false;
      description = "Enable verbose logging";
    };
    
    enableGui = mkOption {
      type = types.bool;
      default = false;
      description = "Enable GUI mode (requires X11/Wayland)";
    };
    
    enableNotifications = mkOption {
      type = types.bool;
      default = false;
      description = "Enable desktop notifications";
    };
    
    bluetoothAdapter = mkOption {
      type = types.str;
      default = "hci0";
      description = "Bluetooth adapter to use";
    };
    
    extraArgs = mkOption {
      type = types.listOf types.str;
      default = [];
      description = "Additional command-line arguments";
    };
    
    openFirewall = mkOption {
      type = types.bool;
      default = false;
      description = "Open firewall ports for future WiFi Direct support";
    };
    
    enableHealthMonitoring = mkOption {
      type = types.bool;
      default = true;
      description = "Enable health monitoring service and timer";
    };
    
    healthCheckInterval = mkOption {
      type = types.str;
      default = "5min";
      description = "Interval for health checks (systemd time format)";
    };
  };
  
  config = mkIf cfg.enable {
    # Create user and group
    users.users.${cfg.user} = {
      isSystemUser = true;
      group = cfg.group;
      description = "BitChat daemon user";
      home = cfg.dataDir;
      createHome = true;
      extraGroups = [ "bluetooth" ];
    };
    
    users.groups.${cfg.group} = {};
    
    # Enable Bluetooth with proper configuration
    hardware.bluetooth = {
      enable = true;
      powerOnBoot = true;
      settings = {
        General = {
          Enable = "Source,Sink,Media,Socket";
          Experimental = true;  # Enable experimental features for better BLE support
        };
        Policy = {
          AutoEnable = true;
        };
      };
    };
    
    # Enable BlueZ and related services
    services.blueman.enable = mkDefault true;
    services.dbus.enable = true;
    
    # Ensure proper udev rules for Bluetooth
    services.udev.extraRules = ''
      # Allow bitchat group to access Bluetooth devices
      SUBSYSTEM=="bluetooth", GROUP="bluetooth", MODE="0664"
      KERNEL=="hci[0-9]*", GROUP="bluetooth", MODE="0664"
      
      # Additional rules for BLE devices
      SUBSYSTEM=="bluetooth", ATTR{type}=="1", GROUP="bluetooth", MODE="0664"
      KERNEL=="rfkill", GROUP="bluetooth", MODE="0664"
    '';
    
    # Create systemd service
    systemd.services.bitchat = {
      description = "BitChat Decentralized Messaging Daemon";
      documentation = [ "https://github.com/your-repo/bitchat" ];
      after = [ "bluetooth.service" "network.target" "dbus.service" ];
      requires = [ "bluetooth.service" "dbus.service" ];
      wants = [ "network.target" ];
      wantedBy = [ "multi-user.target" ];
      
      serviceConfig = {
        Type = "simple";
        User = cfg.user;
        Group = cfg.group;
        Restart = "always";
        RestartSec = "10";
        RestartPreventExitStatus = "SIGKILL";
        
        # Working directory
        WorkingDirectory = cfg.dataDir;
        
        # Security settings
        NoNewPrivileges = true;
        PrivateTmp = true;
        PrivateDevices = false;  # Need access to Bluetooth devices
        ProtectSystem = "strict";
        ProtectHome = true;
        ProtectKernelTunables = true;
        ProtectKernelModules = true;
        ProtectControlGroups = true;
        ReadWritePaths = [ cfg.dataDir "/var/log/bitchat" ];
        
        # Capabilities for Bluetooth access
        CapabilityBoundingSet = [ "CAP_NET_RAW" "CAP_NET_ADMIN" ];
        AmbientCapabilities = [ "CAP_NET_RAW" ];
        
        # Resource limits
        LimitNOFILE = "65536";
        LimitNPROC = "4096";
        MemoryMax = "512M";
        
        # Environment
        Environment = [
          "HOME=${cfg.dataDir}"
          "XDG_CONFIG_HOME=${cfg.dataDir}/.config"
          "XDG_DATA_HOME=${cfg.dataDir}/.local/share"
          "XDG_CACHE_HOME=${cfg.dataDir}/.cache"
          "QT_QPA_PLATFORM=offscreen"  # Headless Qt
          "BITCHAT_SCRIPTS_DIR=/etc/bitchat/scripts"
          "BITCHAT_BUILD_SYSTEM=nixos"
        ] ++ optionals cfg.enableGui [
          "DISPLAY=:0"
          "QT_QPA_PLATFORM=xcb"
        ];
        
        # Command
        ExecStart = "${cfg.package}/bin/bitchat-linux" +
                   (if cfg.enableGui then "" else " --daemon") +
                   optionalString cfg.verbose " --verbose" +
                   optionalString (cfg.configFile != null) " --config ${cfg.configFile}" +
                   optionalString (cfg.configFile == null) " --config ${configFile}" +
                   optionalString (cfg.extraArgs != []) " ${concatStringsSep " " cfg.extraArgs}";
        
        # Pre-start checks
        ExecStartPre = [
          "${pkgs.coreutils}/bin/mkdir -p ${cfg.dataDir}/.config/bitchat"
          "${pkgs.coreutils}/bin/mkdir -p ${cfg.dataDir}/.local/share/bitchat"
          "${pkgs.coreutils}/bin/mkdir -p ${cfg.dataDir}/.cache/bitchat"
          "${pkgs.coreutils}/bin/mkdir -p /var/log/bitchat"
          "${pkgs.systemd}/bin/systemctl is-active bluetooth.service"
        ];
        
        # Health check
        ExecReload = "${pkgs.coreutils}/bin/kill -USR1 $MAINPID";
        
        # Logging
        StandardOutput = "journal";
        StandardError = "journal";
        SyslogIdentifier = "bitchat";
        
        # Watchdog
        WatchdogSec = "30";
        NotifyAccess = "main";
      };
      
      # Ensure Bluetooth is ready
      preStart = ''
        echo "🔍 Checking BitChat prerequisites..."
        
        # Wait for Bluetooth adapter
        timeout=30
        while [ $timeout -gt 0 ]; do
          if ${pkgs.bluez}/bin/hciconfig ${cfg.bluetoothAdapter} >/dev/null 2>&1; then
            echo "✅ Bluetooth adapter ${cfg.bluetoothAdapter} is ready"
            break
          fi
          echo "⏳ Waiting for Bluetooth adapter ${cfg.bluetoothAdapter}..."
          sleep 1
          timeout=$((timeout - 1))
        done
        
        if [ $timeout -eq 0 ]; then
          echo "❌ ERROR: Bluetooth adapter ${cfg.bluetoothAdapter} not found"
          exit 1
        fi
        
        # Verify adapter is up
        if ! ${pkgs.bluez}/bin/hciconfig ${cfg.bluetoothAdapter} | grep -q "UP RUNNING"; then
          echo "🔧 Bringing up Bluetooth adapter ${cfg.bluetoothAdapter}..."
          ${pkgs.bluez}/bin/hciconfig ${cfg.bluetoothAdapter} up || {
            echo "❌ ERROR: Failed to bring up Bluetooth adapter"
            exit 1
          }
        fi
        
        # Ensure proper permissions
        ${pkgs.coreutils}/bin/chown -R ${cfg.user}:${cfg.group} ${cfg.dataDir}
        ${pkgs.coreutils}/bin/chmod 755 ${cfg.dataDir}
        ${pkgs.coreutils}/bin/chown -R ${cfg.user}:${cfg.group} /var/log/bitchat
        ${pkgs.coreutils}/bin/chmod 755 /var/log/bitchat
        
        echo "✅ BitChat prerequisites verified"
      '';
    };
    
    # Set capabilities on the binary using a wrapper
    security.wrappers.bitchat-linux = {
      source = "${cfg.package}/bin/bitchat-linux";
      capabilities = "cap_net_raw+eip";
      owner = "root";
      group = "root";
      permissions = "u+rx,g+rx,o+rx";
    };
    
    # Add to system packages
    environment.systemPackages = [ cfg.package ];
    
    # Create configuration directory structure
    systemd.tmpfiles.rules = [
      "d ${cfg.dataDir} 0755 ${cfg.user} ${cfg.group} -"
      "d ${cfg.dataDir}/.config 0755 ${cfg.user} ${cfg.group} -"
      "d ${cfg.dataDir}/.config/bitchat 0755 ${cfg.user} ${cfg.group} -"
      "d ${cfg.dataDir}/.local 0755 ${cfg.user} ${cfg.group} -"
      "d ${cfg.dataDir}/.local/share 0755 ${cfg.user} ${cfg.group} -"
      "d ${cfg.dataDir}/.local/share/bitchat 0755 ${cfg.user} ${cfg.group} -"
      "d ${cfg.dataDir}/.cache 0755 ${cfg.user} ${cfg.group} -"
      "d ${cfg.dataDir}/.cache/bitchat 0755 ${cfg.user} ${cfg.group} -"
      "d /var/log/bitchat 0755 ${cfg.user} ${cfg.group} -"
      "d /etc/bitchat 0755 root root -"
      "d /etc/bitchat/scripts 0755 root root -"
    ];
    
    # Logrotate configuration
    services.logrotate.settings.bitchat = {
      files = "/var/log/bitchat/*.log";
      frequency = "daily";
      rotate = 7;
      compress = true;
      delaycompress = true;
      missingok = true;
      notifempty = true;
      create = "644 ${cfg.user} ${cfg.group}";
      postrotate = "systemctl reload bitchat.service";
    };
    
    # Firewall configuration for future WiFi Direct support
    networking.firewall = mkIf cfg.openFirewall {
      allowedTCPPorts = [ 8080 8443 ];  # HTTP and HTTPS for WiFi Direct
      allowedUDPPorts = [ 5353 ];       # mDNS for service discovery
    };
    
    # Desktop integration (if GUI enabled)
    services.xserver = mkIf cfg.enableGui {
      enable = true;
      desktopManager.xterm.enable = false;
    };
    
    # Notification support
    services.dbus.packages = mkIf cfg.enableNotifications [ pkgs.libnotify ];
    
    # Bluetooth debugging tools (optional)
    environment.systemPackages = with pkgs; [
      bluez-tools
      btop
    ];
    
    # Add user to bluetooth group for non-root access
    users.groups.bluetooth.members = [ cfg.user ];
    
    # Ensure proper D-Bus configuration
    services.dbus.enable = true;
    
    # Journal configuration for better logging
    services.journald.extraConfig = ''
      SystemMaxUse=100M
      SystemMaxFileSize=10M
      SystemMaxFiles=10
      MaxRetentionSec=7day
    '';
    
    # Enhanced health monitoring and management
    systemd.services.bitchat-health = mkIf cfg.enableHealthMonitoring {
      description = "BitChat Health Monitor";
      after = [ "bitchat.service" ];
      requires = [ "bitchat.service" ];
      
      serviceConfig = {
        Type = "oneshot";
        User = cfg.user;
        Group = cfg.group;
        ExecStart = pkgs.writeShellScript "bitchat-health" ''
          echo "🩺 BitChat Health Check - $(date)"
          
          # Check if BitChat is responding
          if ! pgrep -f "bitchat-linux" > /dev/null; then
            echo "❌ BitChat process not found"
            exit 1
          fi
          
          # Check Bluetooth adapter
          if ! ${pkgs.bluez}/bin/hciconfig ${cfg.bluetoothAdapter} | grep -q "UP RUNNING"; then
            echo "❌ Bluetooth adapter not running"
            exit 1
          fi
          
          # Check memory usage
          memory_usage=$(ps -o pid,ppid,cmd,%mem --sort=-%mem | grep bitchat-linux | head -1 | awk '{print $4}')
          if [ -n "$memory_usage" ]; then
            echo "📊 Memory usage: $memory_usage%"
            if (( $(echo "$memory_usage > 80" | bc -l) )); then
              echo "⚠️  High memory usage detected"
            fi
          fi
          
          # Check log file size
          log_size=$(du -sh /var/log/bitchat/ 2>/dev/null | cut -f1 || echo "0")
          echo "📝 Log directory size: $log_size"
          
          # Check configuration
          if [ -f "${configFile}" ]; then
            echo "✅ Configuration file exists"
          else
            echo "⚠️  Configuration file not found"
          fi
          
          echo "✅ BitChat health check passed"
        '';
      };
    };
    
    # Timer for health checks
    systemd.timers.bitchat-health = mkIf cfg.enableHealthMonitoring {
      description = "BitChat Health Check Timer";
      wantedBy = [ "timers.target" ];
      timerConfig = {
        OnBootSec = "5min";
        OnUnitActiveSec = cfg.healthCheckInterval;
        Unit = "bitchat-health.service";
      };
    };
    
    # Install organized scripts for easy management
    environment.etc."bitchat/scripts".source = "${cfg.package}/share/bitchat/scripts";
    
    # Create convenience aliases and management scripts
    environment.shellAliases = {
      bitchat-status = "systemctl status bitchat";
      bitchat-logs = "journalctl -u bitchat -f";
      bitchat-restart = "sudo systemctl restart bitchat";
      bitchat-test = "sudo -u ${cfg.user} /etc/bitchat/scripts/nixos/nixos-test.sh";
      bitchat-health = "systemctl status bitchat-health";
    };
    
    # Create management wrapper script
    environment.systemPackages = [
      (pkgs.writeScriptBin "bitchat-manager" ''
        #!/bin/bash
        
        case "$1" in
          status)
            echo "📊 BitChat Service Status"
            systemctl status bitchat
            echo ""
            echo "🔍 Health Check Status"
            systemctl status bitchat-health
            ;;
          logs)
            journalctl -u bitchat -f
            ;;
          test)
            if [ -f /etc/bitchat/scripts/nixos/nixos-test.sh ]; then
              sudo -u ${cfg.user} /etc/bitchat/scripts/nixos/nixos-test.sh "$@"
            else
              echo "❌ Test script not found"
            fi
            ;;
          restart)
            sudo systemctl restart bitchat
            ;;
          health)
            systemctl start bitchat-health
            ;;
          *)
            echo "BitChat Manager - NixOS Service Management"
            echo ""
            echo "Usage: bitchat-manager <command>"
            echo ""
            echo "Commands:"
            echo "  status   - Show service status"
            echo "  logs     - Show live logs"
            echo "  test     - Run test suite"
            echo "  restart  - Restart service"
            echo "  health   - Run health check"
            echo ""
            echo "Aliases available:"
            echo "  bitchat-status, bitchat-logs, bitchat-restart"
            echo "  bitchat-test, bitchat-health"
            ;;
        esac
      '')
    ];
  };
} 