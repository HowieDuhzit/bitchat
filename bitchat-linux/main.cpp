#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/capability.h>

#include "BitchatApplication.h"
// #include "ui/MainWindow.h"  // TODO: Implement GUI

static BitchatApplication* g_app = nullptr;

void signalHandler(int signal) {
    if (g_app) {
        std::cout << "Received signal " << signal << ", shutting down gracefully..." << std::endl;
        g_app->shutdown();
    }
    QApplication::quit();
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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application properties
    QApplication::setApplicationName("bitchat");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("bitchat");
    QApplication::setOrganizationDomain("chat.bitchat");
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Decentralized mesh messaging over Bluetooth LE");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption daemonOption("daemon", "Run as background daemon");
    parser.addOption(daemonOption);
    
    QCommandLineOption configOption("config", "Configuration file path", "path");
    parser.addOption(configOption);
    
    QCommandLineOption verboseOption("verbose", "Enable verbose logging");
    parser.addOption(verboseOption);
    
    QCommandLineOption guiOption("gui", "Run with GUI (not implemented yet)");
    parser.addOption(guiOption);
    
    parser.process(app);
    
    // Check for Bluetooth capabilities
    if (!checkBluetoothCapabilities()) {
        if (geteuid() != 0) {
            std::cerr << "Error: bitchat requires CAP_NET_RAW capability for Bluetooth LE." << std::endl;
            std::cerr << "Please run:" << std::endl;
            std::cerr << "sudo setcap cap_net_raw+eip " << QApplication::applicationFilePath().toStdString() << std::endl;
            std::cerr << "Or run as root (not recommended)." << std::endl;
            return 1;
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
        std::cerr << "Error: Failed to initialize bitchat. Check that Bluetooth is available and enabled." << std::endl;
        return 1;
    }
    
    // Check if GUI is requested
    if (parser.isSet(guiOption)) {
        std::cerr << "Error: GUI mode is not implemented yet. Use --daemon or run without --gui." << std::endl;
        return 1;
    }
    
    // Run as daemon or console application
    if (parser.isSet(daemonOption)) {
        std::cout << "Starting bitchat daemon..." << std::endl;
        bitchatApp.run();
        return app.exec();
    } else {
        // Console mode - just run the daemon but with interactive output
        std::cout << "Starting bitchat in console mode..." << std::endl;
        std::cout << "Type 'help' for commands or press Ctrl+C to exit." << std::endl;
        bitchatApp.run();
        return app.exec();
    }
} 