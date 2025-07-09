#include <QCoreApplication>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    std::cout << "BitChat Test Application" << std::endl;
    std::cout << "========================" << std::endl;
    
    // Test basic Qt functionality
    std::cout << "Qt version: " << qVersion() << std::endl;
    std::cout << "Application name: " << qApp->applicationName().toStdString() << std::endl;
    
    // Simple command line interface
    std::cout << "\nType 'help' for commands or 'quit' to exit:" << std::endl;
    
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit" || input == "exit") {
            break;
        } else if (input == "help") {
            std::cout << "Available commands:" << std::endl;
            std::cout << "  help - Show this help" << std::endl;
            std::cout << "  quit/exit - Exit application" << std::endl;
            std::cout << "  status - Show application status" << std::endl;
        } else if (input == "status") {
            std::cout << "Application status: Running" << std::endl;
            std::cout << "Bluetooth: Not initialized (test mode)" << std::endl;
            std::cout << "Peers connected: 0" << std::endl;
        } else if (!input.empty()) {
            std::cout << "Unknown command: " << input << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
        
        std::cout << "\n> ";
    }
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
} 