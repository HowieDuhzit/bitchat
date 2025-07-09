#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <signal.h>

class BitChatTerminal {
private:
    std::string nickname;
    std::vector<std::string> channels;
    std::vector<std::string> peers;
    bool isRunning;
    bool mockMode;
    
public:
    BitChatTerminal() : isRunning(false), mockMode(false) {
        nickname = "User" + std::to_string(rand() % 1000);
    }
    
    void setMockMode(bool mock) {
        mockMode = mock;
    }
    
    void setNickname(const std::string& nick) {
        nickname = nick;
        std::cout << "Nickname set to: " << nickname << std::endl;
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
        std::cout << "  /clear             - Clear chat messages" << std::endl;
        std::cout << "  /status            - Show connection status" << std::endl;
        std::cout << "  /quit              - Exit application" << std::endl;
        std::cout << std::endl;
    }
    
    void joinChannel(const std::string& channel) {
        if (channel.empty()) {
            std::cout << "Error: Channel name required" << std::endl;
            return;
        }
        
        std::string cleanChannel = channel;
        if (cleanChannel[0] == '#') {
            cleanChannel = cleanChannel.substr(1);
        }
        
        channels.push_back(cleanChannel);
        std::cout << "Joined channel: #" << cleanChannel << std::endl;
    }
    
    void sendMessage(const std::string& message, const std::string& target = "") {
        if (message.empty()) {
            std::cout << "Error: Message cannot be empty" << std::endl;
            return;
        }
        
        if (target.empty()) {
            // Public message
            std::cout << "[" << nickname << "] " << message << std::endl;
        } else {
            // Private message
            std::cout << "[" << nickname << " -> " << target << "] " << message << std::endl;
        }
    }
    
    void listPeers() {
        if (peers.empty()) {
            std::cout << "No peers connected" << std::endl;
        } else {
            std::cout << "Connected peers:" << std::endl;
            for (const auto& peer : peers) {
                std::cout << "  - " << peer << std::endl;
            }
        }
    }
    
    void listChannels() {
        if (channels.empty()) {
            std::cout << "No channels joined" << std::endl;
        } else {
            std::cout << "Joined channels:" << std::endl;
            for (const auto& channel : channels) {
                std::cout << "  #" << channel << std::endl;
            }
        }
    }
    
    void showStatus() {
        std::cout << "Connection Status:" << std::endl;
        std::cout << "  Nickname: " << nickname << std::endl;
        std::cout << "  Mode: " << (mockMode ? "Mock (no Bluetooth)" : "Normal") << std::endl;
        std::cout << "  Peers connected: " << peers.size() << std::endl;
        std::cout << "  Channels joined: " << channels.size() << std::endl;
        std::cout << "  Status: " << (isRunning ? "Running" : "Stopped") << std::endl;
    }
    
    void clearScreen() {
        std::cout << "\033[2J\033[1;1H"; // Clear screen and move cursor to top
    }
    
    void processCommand(const std::string& input) {
        if (input.empty()) return;
        
        if (input[0] == '/') {
            // Command
            std::string cmd = input.substr(1);
            size_t spacePos = cmd.find(' ');
            std::string command = spacePos != std::string::npos ? cmd.substr(0, spacePos) : cmd;
            std::string args = spacePos != std::string::npos ? cmd.substr(spacePos + 1) : "";
            
            if (command == "help") {
                showHelp();
            } else if (command == "j" || command == "join") {
                joinChannel(args);
            } else if (command == "m" || command == "msg") {
                size_t atPos = args.find('@');
                if (atPos != std::string::npos) {
                    size_t msgStart = args.find(' ', atPos);
                    if (msgStart != std::string::npos) {
                        std::string target = args.substr(atPos + 1, msgStart - atPos - 1);
                        std::string message = args.substr(msgStart + 1);
                        sendMessage(message, target);
                    } else {
                        std::cout << "Error: Message required" << std::endl;
                    }
                } else {
                    std::cout << "Error: Use @username to send private message" << std::endl;
                }
            } else if (command == "w" || command == "who") {
                listPeers();
            } else if (command == "channels") {
                listChannels();
            } else if (command == "peers") {
                listPeers();
            } else if (command == "nick") {
                if (!args.empty()) {
                    setNickname(args);
                } else {
                    std::cout << "Error: Nickname required" << std::endl;
                }
            } else if (command == "clear") {
                clearScreen();
                showBanner();
            } else if (command == "status") {
                showStatus();
            } else if (command == "quit" || command == "exit") {
                isRunning = false;
            } else {
                std::cout << "Unknown command: /" << command << std::endl;
                std::cout << "Type /help for available commands" << std::endl;
            }
        } else {
            // Regular message
            sendMessage(input);
        }
    }
    
    void run() {
        isRunning = true;
        showBanner();
        
        if (mockMode) {
            std::cout << "Running in mock mode (no Bluetooth functionality)" << std::endl;
        }
        
        std::cout << "Type '/help' for commands or press Ctrl+C to exit." << std::endl;
        std::cout << std::endl;
        
        std::string input;
        while (isRunning && std::getline(std::cin, input)) {
            processCommand(input);
            std::cout << "\n> ";
        }
        
        std::cout << "Goodbye!" << std::endl;
    }
};

static BitChatTerminal* g_app = nullptr;

void signalHandler(int signal) {
    if (g_app) {
        std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool mockMode = false;
    bool verbose = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--mock" || arg == "-m") {
            mockMode = true;
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --mock, -m      Run in mock mode (no Bluetooth)" << std::endl;
            std::cout << "  --verbose, -v    Enable verbose output" << std::endl;
            std::cout << "  --help, -h       Show this help" << std::endl;
            return 0;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Create and run application
    BitChatTerminal app;
    g_app = &app;
    
    app.setMockMode(mockMode);
    app.run();
    
    return 0;
} 