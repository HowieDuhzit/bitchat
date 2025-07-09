#include "MessageHandler.h"
#include <QDebug>
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>
#include <sstream>

MessageHandler::MessageHandler(QObject *parent)
    : QObject(parent)
    , m_currentChannel("general")
    , m_messageIdCounter(0)
{
    // Initialize supported commands
    initializeCommands();
}

MessageHandler::~MessageHandler() {
}

void MessageHandler::processMessage(const std::string& input) {
    if (input.empty()) {
        return;
    }
    
    // Check if it's a command (starts with /)
    if (input[0] == '/') {
        processCommand(input);
    } else {
        // Regular message
        sendMessage(input, m_currentChannel);
    }
}

void MessageHandler::processIncomingMessage(const std::vector<uint8_t>& data, const std::string& senderId) {
    if (data.empty()) {
        return;
    }
    
    try {
        auto messagePtr = BitchatMessage::fromBinaryPayload(data);
        if (!messagePtr) {
            qDebug() << "Failed to parse message from" << QString::fromStdString(senderId);
            return;
        }
        
        BitchatMessage message = *messagePtr;
        message.sender = senderId;
        message.timestamp = getCurrentTimestamp();
        
        // Validate message
        if (!validateMessage(message)) {
            qDebug() << "Invalid message received from" << QString::fromStdString(senderId);
            return;
        }
        
        // Process based on message content and channel
        if (!message.channel.empty()) {
            handleChannelMessage(message);
        } else if (message.isPrivate) {
            handlePrivateMessage(message);
        } else {
                handleChatMessage(message);
        }
        
        // Store message in history
        addToHistory(message);
        
    } catch (const std::exception& e) {
        qDebug() << "Error processing incoming message:" << e.what();
    }
}

void MessageHandler::sendMessage(const std::string& content, const std::string& channel) {
    if (content.empty()) {
        return;
    }
    
    BitchatMessage message;
    message.id = generateMessageId();
    message.content = content;
    message.channel = channel;
    message.sender = m_nickname;
    message.timestamp = getCurrentTimestamp();
    message.isPrivate = false;
    
    // Convert to binary format
    std::vector<uint8_t> data = message.toBinaryPayload();
    
    // Send via mesh service
    if (channel.empty()) {
        emit messageToBroadcast(data);
    } else {
        emit messageToChannel(data, channel);
    }
    
    // Add to local history
    addToHistory(message);
    
    // Emit for UI update
    emit messageProcessed(message);
}

void MessageHandler::sendPrivateMessage(const std::string& content, const std::string& recipientId) {
    if (content.empty() || recipientId.empty()) {
        return;
    }
    
    BitchatMessage message;
    message.id = generateMessageId();
    message.content = content;
    message.sender = m_nickname;
    message.recipientNickname = recipientId;
    message.timestamp = getCurrentTimestamp();
    message.isPrivate = true;
    
    // Convert to binary format
    std::vector<uint8_t> data = message.toBinaryPayload();
    
    // Send via mesh service
    emit privateMessageToPeer(data, recipientId);
    
    // Add to local history
    addToHistory(message);
    
    // Emit for UI update
    emit messageProcessed(message);
}

void MessageHandler::joinChannel(const std::string& channel) {
    if (channel.empty() || channel[0] != '#') {
        emit commandResult("Invalid channel name. Channels must start with #");
        return;
    }
    
    m_currentChannel = channel;
    m_joinedChannels.insert(channel);
    
    // Send join notification
    BitchatMessage message;
    message.id = generateMessageId();
    message.content = "joined channel";
    message.channel = channel;
    message.sender = m_nickname;
    message.timestamp = getCurrentTimestamp();
    
    std::vector<uint8_t> data = message.toBinaryPayload();
    emit messageToChannel(data, channel);
    
    emit commandResult("Joined channel: " + channel);
    emit channelJoined(channel);
}

void MessageHandler::leaveChannel(const std::string& channel) {
    if (channel.empty()) {
        return;
    }
    
    m_joinedChannels.erase(channel);
    
    // Send leave notification
    BitchatMessage message;
    message.id = generateMessageId();
    message.content = "left channel";
    message.channel = channel;
    message.sender = m_nickname;
    message.timestamp = getCurrentTimestamp();
    
    std::vector<uint8_t> data = message.toBinaryPayload();
    emit messageToChannel(data, channel);
    
    // Switch to general if leaving current channel
    if (m_currentChannel == channel) {
        m_currentChannel = "general";
    }
    
    emit commandResult("Left channel: " + channel);
    emit channelLeft(channel);
}

void MessageHandler::setNickname(const std::string& nickname) {
    if (nickname.empty()) {
        return;
    }
    
    m_nickname = nickname;
    emit nicknameChanged(nickname);
    emit commandResult("Nickname set to: " + nickname);
}

std::string MessageHandler::getNickname() const {
    return m_nickname;
}

std::string MessageHandler::getCurrentChannel() const {
    return m_currentChannel;
}

std::set<std::string> MessageHandler::getJoinedChannels() const {
    return m_joinedChannels;
}

std::vector<BitchatMessage> MessageHandler::getMessageHistory(const std::string& channel, int limit) const {
    std::vector<BitchatMessage> result;
    
    // Filter messages by channel
    for (const auto& message : m_messageHistory) {
        if (channel.empty() || message.channel == channel) {
            result.push_back(message);
        }
    }
    
    // Sort by timestamp
    std::sort(result.begin(), result.end(), [](const BitchatMessage& a, const BitchatMessage& b) {
        return a.timestamp < b.timestamp;
    });
    
    // Apply limit
    if (limit > 0 && result.size() > static_cast<size_t>(limit)) {
        result.erase(result.begin(), result.end() - limit);
    }
    
    return result;
}

void MessageHandler::clearHistory() {
    m_messageHistory.clear();
    emit historyCleared();
}

// Private methods
void MessageHandler::initializeCommands() {
    m_commands = {
        {"help", [this](const std::string& args) { handleHelpCommand(args); }},
        {"join", [this](const std::string& args) { handleJoinCommand(args); }},
        {"leave", [this](const std::string& args) { handleLeaveCommand(args); }},
        {"msg", [this](const std::string& args) { handleMessageCommand(args); }},
        {"nick", [this](const std::string& args) { handleNickCommand(args); }},
        {"list", [this](const std::string& args) { handleListCommand(args); }},
        {"channels", [this](const std::string& args) { handleChannelsCommand(args); }},
        {"clear", [this](const std::string& args) { handleClearCommand(args); }},
        {"quit", [this](const std::string& args) { handleQuitCommand(args); }},
        {"status", [this](const std::string& args) { handleStatusCommand(args); }},
        {"history", [this](const std::string& args) { handleHistoryCommand(args); }},
        {"block", [this](const std::string& args) { handleBlockCommand(args); }},
        {"unblock", [this](const std::string& args) { handleUnblockCommand(args); }},
        {"info", [this](const std::string& args) { handleInfoCommand(args); }},
        {"relay", [this](const std::string& args) { handleRelayCommand(args); }},
        {"encrypt", [this](const std::string& args) { handleEncryptCommand(args); }}
    };
}

void MessageHandler::processCommand(const std::string& input) {
    // Parse command and arguments
    std::istringstream iss(input.substr(1)); // Remove leading '/'
    std::string command;
    iss >> command;
    
    std::string args;
    std::getline(iss, args);
    if (!args.empty() && args[0] == ' ') {
        args = args.substr(1); // Remove leading space
    }
    
    // Convert to lowercase
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);
    
    // Execute command
    if (m_commands.count(command)) {
        m_commands[command](args);
    } else {
        emit commandResult("Unknown command: " + command + ". Type /help for available commands.");
    }
}

void MessageHandler::handleHelpCommand(const std::string& args) {
    std::string result = "Available commands:\n";
    result += "/help - Show available commands\n";
    result += "/join #channel - Join a channel\n";
    result += "/leave #channel - Leave a channel\n";
    result += "/msg <peer> <message> - Send private message\n";
    result += "/nick <nickname> - Set nickname\n";
    result += "/list - List connected peers\n";
    result += "/channels - List joined channels\n";
    result += "/clear - Clear message history\n";
    result += "/quit - Quit the application\n";
    result += "/status - Show connection status\n";
    result += "/history [channel] [limit] - Show message history\n";
    result += "/block <peer> - Block a peer\n";
    result += "/unblock <peer> - Unblock a peer\n";
    result += "/info <peer> - Show peer info\n";
    result += "/relay [on|off] - Toggle message relay\n";
    result += "/encrypt [on|off] - Toggle encryption\n";
    emit commandResult(result);
}

void MessageHandler::handleJoinCommand(const std::string& args) {
    if (args.empty()) {
        emit commandResult("Usage: /join #channel");
        return;
    }
    
    std::string channel = args;
    if (channel[0] != '#') {
        channel = "#" + channel;
    }
    
    joinChannel(channel);
}

void MessageHandler::handleLeaveCommand(const std::string& args) {
    std::string channel = args.empty() ? m_currentChannel : args;
    if (channel[0] != '#') {
        channel = "#" + channel;
    }
    
    leaveChannel(channel);
}

void MessageHandler::handleMessageCommand(const std::string& args) {
    std::istringstream iss(args);
    std::string peerId;
    iss >> peerId;
    
    std::string message;
    std::getline(iss, message);
    if (!message.empty() && message[0] == ' ') {
        message = message.substr(1);
    }
    
    if (peerId.empty() || message.empty()) {
        emit commandResult("Usage: /msg <peer> <message>");
        return;
    }
    
    sendPrivateMessage(message, peerId);
}

void MessageHandler::handleNickCommand(const std::string& args) {
    if (args.empty()) {
        emit commandResult("Current nickname: " + m_nickname);
        return;
    }
    
    setNickname(args);
}

void MessageHandler::handleListCommand(const std::string& args) {
    emit requestPeerList();
}

void MessageHandler::handleChannelsCommand(const std::string& args) {
    if (m_joinedChannels.empty()) {
        emit commandResult("No channels joined.");
        return;
    }
    
    std::string result = "Joined channels:\n";
    for (const auto& channel : m_joinedChannels) {
        result += channel;
        if (channel == m_currentChannel) {
            result += " (current)";
        }
        result += "\n";
    }
    emit commandResult(result);
}

void MessageHandler::handleClearCommand(const std::string& args) {
    clearHistory();
    emit commandResult("Message history cleared.");
}

void MessageHandler::handleQuitCommand(const std::string& args) {
    emit quitRequested();
}

void MessageHandler::handleStatusCommand(const std::string& args) {
    emit requestConnectionStatus();
}

void MessageHandler::handleHistoryCommand(const std::string& args) {
    std::istringstream iss(args);
    std::string channel;
    int limit = 50; // Default limit
    
    iss >> channel;
    if (iss >> limit) {
        // Limit was provided
    }
    
    std::vector<BitchatMessage> history = getMessageHistory(channel, limit);
    
    if (history.empty()) {
        emit commandResult("No message history found.");
        return;
    }
    
    std::string result = "Message history:\n";
    for (const auto& msg : history) {
        result += "[" + std::to_string(msg.timestamp) + "] ";
        if (!msg.sender.empty()) {
            result += msg.sender + ": ";
        }
        result += msg.content + "\n";
    }
    
    emit commandResult(result);
}

void MessageHandler::handleBlockCommand(const std::string& args) {
    if (args.empty()) {
        emit commandResult("Usage: /block <peer>");
        return;
    }
    
    emit blockPeerRequested(args);
}

void MessageHandler::handleUnblockCommand(const std::string& args) {
    if (args.empty()) {
        emit commandResult("Usage: /unblock <peer>");
        return;
    }
    
    emit unblockPeerRequested(args);
}

void MessageHandler::handleInfoCommand(const std::string& args) {
    if (args.empty()) {
        emit commandResult("Usage: /info <peer>");
        return;
    }
    
    emit requestPeerInfo(args);
}

void MessageHandler::handleRelayCommand(const std::string& args) {
    if (args.empty()) {
        emit requestRelayStatus();
        return;
    }
    
    bool enable = (args == "on" || args == "true" || args == "1");
    emit setRelayEnabled(enable);
}

void MessageHandler::handleEncryptCommand(const std::string& args) {
    if (args.empty()) {
        emit requestEncryptionStatus();
        return;
    }
    
    bool enable = (args == "on" || args == "true" || args == "1");
    emit setEncryptionEnabled(enable);
}

BitchatMessage MessageHandler::parseMessage(const std::vector<uint8_t>& data) {
    auto messagePtr = BitchatMessage::fromBinaryPayload(data);
    if (!messagePtr) {
        throw std::runtime_error("Failed to parse message");
    }
    return *messagePtr;
}

std::vector<uint8_t> MessageHandler::serializeMessage(const BitchatMessage& message) {
    return message.toBinaryPayload();
}

std::string MessageHandler::readString(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset >= data.size()) {
        throw std::runtime_error("Unexpected end of data");
    }
    
    // Find null terminator
    size_t start = offset;
    while (offset < data.size() && data[offset] != 0) {
        offset++;
    }
    
    if (offset >= data.size()) {
        throw std::runtime_error("String not null-terminated");
    }
    
    std::string result(data.begin() + start, data.begin() + offset);
    offset++; // Skip null terminator
    
    return result;
}

void MessageHandler::writeString(std::vector<uint8_t>& data, const std::string& str) {
    data.insert(data.end(), str.begin(), str.end());
    data.push_back(0); // Null terminator
}

bool MessageHandler::validateMessage(const BitchatMessage& message) {
    // Basic validation
    if (message.sender.empty()) {
        qDebug() << "Message has no sender";
        return false;
    }
    
    // Content validation
    if (message.content.empty()) {
        qDebug() << "Message has no content";
        return false;
    }
    
    // Private message validation
    if (message.isPrivate && message.recipientNickname.empty()) {
        qDebug() << "Private message has no recipient";
        return false;
    }
    
    // Channel message validation
    if (!message.channel.empty() && message.channel.length() > 50) {
        qDebug() << "Channel name too long";
                return false;
    }
    
    return true;
}

void MessageHandler::handleChatMessage(const BitchatMessage& message) {
    emit messageReceived(message);
}

void MessageHandler::handlePrivateMessage(const BitchatMessage& message) {
    emit privateMessageReceived(message);
}

void MessageHandler::handleChannelMessage(const BitchatMessage& message) {
    emit channelMessageReceived(message);
}

void MessageHandler::handleSystemMessage(const BitchatMessage& message) {
    emit systemMessageReceived(message);
}

void MessageHandler::handleRelayMessage(const BitchatMessage& message) {
    // Relay messages are handled by the mesh service
    // Just emit for UI display
    emit messageReceived(message);
}

void MessageHandler::addToHistory(const BitchatMessage& message) {
    m_messageHistory.push_back(message);
    
    // Limit history size
    if (m_messageHistory.size() > MAX_HISTORY_SIZE) {
        m_messageHistory.erase(m_messageHistory.begin());
    }
}

std::string MessageHandler::generateMessageId() {
    return std::to_string(++m_messageIdCounter);
}

uint64_t MessageHandler::getCurrentTimestamp() {
    return QDateTime::currentMSecsSinceEpoch();
}

std::string MessageHandler::formatMessage(const BitchatMessage& message) {
    std::string formatted = "[" + std::to_string(message.timestamp) + "] ";
    
    if (message.isPrivate) {
        formatted += "*" + message.sender + "* " + message.content;
    } else if (!message.channel.empty()) {
        formatted += message.channel + " <" + message.sender + "> " + message.content;
    } else {
        formatted += "<" + message.sender + "> " + message.content;
    }
    
    return formatted;
} 