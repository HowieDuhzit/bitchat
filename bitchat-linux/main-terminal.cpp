#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/capability.h>

#include "BitchatApplication.h"

static BitchatApplication* g_app = nullptr;

void signalHandler(int signal) {
    if (g_app) {
        std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
        g_app->shutdown();
    }
    QCoreApplication::quit();
}

bool checkBluetoothCapabilities() {
    cap_t caps = cap_get_proc();
    if (!caps) {
        return false;
    }
    
    cap_flag_value_t value;
    cap_get_flag(caps, CAP_NET_RAW, CAP_EFFECTIVE, &value);
    cap_free(caps);
    
    return value == CAP_SET;
}

void setupSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
}

void showBanner() {
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    BitChat Terminal                          ║" << std::endl;
    std::cout << "║              Decentralized Mesh Messaging                   ║" << std::endl;
    std::cout << "║                    Over Bluetooth LE                        ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
}

void showHelp() {
    std::cout << "Available Commands:" << std::endl;
    std::cout << "  /help              - Show this help" << std::endl;
    std::cout << "  /j #channel        - Join or create a channel" << std::endl;
    std::cout << "  /m @name message   - Send a private message" << std::endl;
    std::cout << "  /w                 - List online users" << std::endl;
    std::cout << "  /channels          - Show all discovered channels" << std::endl;
    std::cout << "  /peers             - List connected peers" << std::endl;
    std::cout << "  /nick nickname     - Change your nickname" << std::endl;
    std::cout << "  /block @name       - Block a peer from messaging you" << std::endl;
    std::cout << "  /unblock @name     - Unblock a peer" << std::endl;
    std::cout << "  /clear             - Clear chat messages" << std::endl;
    std::cout << "  /status            - Show connection status" << std::endl;
    std::cout << "  /quit              - Exit application" << std::endl;
    std::cout << std::endl;
    std::cout << "Channel Commands:" << std::endl;
    std::cout << "  /pass [password]   - Set/change channel password (owner only)" << std::endl;
    std::cout << "  /transfer @name    - Transfer channel ownership" << std::endl;
    std::cout << "  /save              - Toggle message retention (owner only)" << std::endl;
    std::cout << "  /leave #channel    - Leave a channel" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    // Set application properties
    QCoreApplication::setApplicationName("bitchat-terminal");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("bitchat");
    QCoreApplication::setOrganizationDomain("chat.bitchat");
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Decentralized mesh messaging over Bluetooth LE - Terminal Version");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption daemonOption("daemon", "Run as background daemon");
    parser.addOption(daemonOption);
    
    QCommandLineOption configOption("config", "Configuration file path", "path");
    parser.addOption(configOption);
    
    QCommandLineOption verboseOption("verbose", "Enable verbose logging");
    parser.addOption(verboseOption);
    
    QCommandLineOption mockOption("mock", "Run in mock mode (no Bluetooth)");
    parser.addOption(mockOption);
    
    parser.process(app);
    
    // Show banner
    showBanner();
    
    // Check for Bluetooth capabilities (unless in mock mode)
    if (!parser.isSet(mockOption) && !checkBluetoothCapabilities()) {
        if (geteuid() != 0) {
            std::cerr << "Warning: Missing CAP_NET_RAW capability for Bluetooth LE." << std::endl;
            std::cerr << "Run with --mock for testing without Bluetooth, or set capabilities:" << std::endl;
            std::cerr << "sudo setcap cap_net_raw+eip " << QCoreApplication::applicationFilePath().toStdString() << std::endl;
            std::cerr << std::endl;
            std::cerr << "Continuing in mock mode..." << std::endl;
        }
    }
    
    // Create config directory if it doesn't exist
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/bitchat";
    QDir().mkpath(configDir);
    
    // Set up signal handlers
    setupSignalHandlers();
    
    // Create main application
    BitchatApplication bitchatApp(argc, argv);
    g_app = &bitchatApp;
    
    // Set verbose mode if requested
    if (parser.isSet(verboseOption)) {
        bitchatApp.setVerbose(true);
    }
    
    // Set custom config path if provided
    if (parser.isSet(configOption)) {
        bitchatApp.setConfigPath(parser.value(configOption).toStdString());
    }
    
    // Initialize the application
    if (!bitchatApp.initialize()) {
        std::cerr << "Error: Failed to initialize bitchat." << std::endl;
        if (!parser.isSet(mockOption)) {
            std::cerr << "Check that Bluetooth is available and enabled, or run with --mock" << std::endl;
        }
        return 1;
    }
    
    // Run as daemon or console application
    if (parser.isSet(daemonOption)) {
        std::cout << "Starting bitchat daemon..." << std::endl;
        bitchatApp.run();
        return app.exec();
    } else {
        // Console mode - interactive terminal interface
        std::cout << "Starting bitchat in terminal mode..." << std::endl;
        if (parser.isSet(mockOption)) {
            std::cout << "Running in mock mode (no Bluetooth functionality)" << std::endl;
        }
        std::cout << "Type '/help' for commands or press Ctrl+C to exit." << std::endl;
        std::cout << std::endl;
        
        bitchatApp.run();
        return app.exec();
    }
} 