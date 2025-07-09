#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <cstdint>
#include <functional>
#include "../Protocols/BinaryProtocol.h"

class MessageHandler : public QObject
{
    Q_OBJECT

public:
    explicit MessageHandler(QObject *parent = nullptr);
    ~MessageHandler();

    // Message processing
    void processMessage(const std::string& input);
    void processIncomingMessage(const std::vector<uint8_t>& data, const std::string& senderId);
    
    // Message sending
    void sendMessage(const std::string& content, const std::string& channel = "");
    void sendPrivateMessage(const std::string& content, const std::string& recipientId);
    
    // Channel management
    void joinChannel(const std::string& channel);
    void leaveChannel(const std::string& channel);
    std::string getCurrentChannel() const;
    std::set<std::string> getJoinedChannels() const;
    
    // User management
    void setNickname(const std::string& nickname);
    std::string getNickname() const;
    
    // Message history
    std::vector<BitchatMessage> getMessageHistory(const std::string& channel = "", int limit = 100) const;
    void clearHistory();

signals:
    // Outgoing messages
    void messageToBroadcast(const std::vector<uint8_t>& data);
    void messageToChannel(const std::vector<uint8_t>& data, const std::string& channel);
    void privateMessageToPeer(const std::vector<uint8_t>& data, const std::string& peerId);
    
    // Incoming messages
    void messageReceived(const BitchatMessage& message);
    void privateMessageReceived(const BitchatMessage& message);
    void channelMessageReceived(const BitchatMessage& message);
    void systemMessageReceived(const BitchatMessage& message);
    void messageProcessed(const BitchatMessage& message);
    
    // Channel events
    void channelJoined(const std::string& channel);
    void channelLeft(const std::string& channel);
    
    // User events
    void nicknameChanged(const std::string& nickname);
    
    // Command results
    void commandResult(const std::string& result);
    
    // System events
    void quitRequested();
    void historyCleared();
    
    // Requests to other services
    void requestPeerList();
    void requestConnectionStatus();
    void requestPeerInfo(const std::string& peerId);
    void requestRelayStatus();
    void requestEncryptionStatus();
    void blockPeerRequested(const std::string& peerId);
    void unblockPeerRequested(const std::string& peerId);
    void setRelayEnabled(bool enabled);
    void setEncryptionEnabled(bool enabled);

private:
    // Command processing
    void processCommand(const std::string& command);
    void initializeCommands();
    
    // Message handling
    void handleChatMessage(const BitchatMessage& message);
    void handlePrivateMessage(const BitchatMessage& message);
    void handleChannelMessage(const BitchatMessage& message);
    void handleSystemMessage(const BitchatMessage& message);
    void handleRelayMessage(const BitchatMessage& message);
    
    // Message serialization
    BitchatMessage parseMessage(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serializeMessage(const BitchatMessage& message);
    
    // Validation
    bool validateMessage(const BitchatMessage& message);
    
    // Utilities
    std::string generateMessageId();
    uint64_t getCurrentTimestamp();
    void addToHistory(const BitchatMessage& message);
    
    // Command handlers
    void handleHelpCommand(const std::string& args);
    void handleJoinCommand(const std::string& args);
    void handleLeaveCommand(const std::string& args);
    void handleMessageCommand(const std::string& args);
    void handleNickCommand(const std::string& args);
    void handleListCommand(const std::string& args);
    void handleChannelsCommand(const std::string& args);
    void handleClearCommand(const std::string& args);
    void handleQuitCommand(const std::string& args);
    void handleStatusCommand(const std::string& args);
    void handleHistoryCommand(const std::string& args);
    void handleBlockCommand(const std::string& args);
    void handleUnblockCommand(const std::string& args);
    void handleInfoCommand(const std::string& args);
    void handleRelayCommand(const std::string& args);
    void handleEncryptCommand(const std::string& args);
    
    // Helper methods
    std::string readString(const std::vector<uint8_t>& data, size_t& offset);
    void writeString(std::vector<uint8_t>& data, const std::string& str);
    std::string formatMessage(const BitchatMessage& message);

private:
    // Constants
    static const size_t MAX_MESSAGE_SIZE = 1024;
    static const size_t MAX_HISTORY_SIZE = 1000;
    
    // Member variables
    std::string m_currentChannel;
    std::set<std::string> m_joinedChannels;
    std::string m_nickname;
    uint64_t m_messageIdCounter;
    
    // Message history
    std::vector<BitchatMessage> m_messageHistory;
    
    // Command map
    std::unordered_map<std::string, std::function<void(const std::string&)>> m_commands;
}; 