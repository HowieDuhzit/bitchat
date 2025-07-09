#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <QDateTime>

// Forward declarations
struct BitchatMessage;
struct BitchatPacket;

// Protocol constants
namespace BitchatProtocol {
    static const uint8_t PROTOCOL_VERSION = 1;
    static const size_t HEADER_SIZE = 13;
    static const size_t SENDER_ID_SIZE = 8;
    static const size_t MAX_PAYLOAD_SIZE = 1024;
    static const uint8_t MAX_TTL = 7;
    
    // Message types
    enum class MessageType : uint8_t {
        ANNOUNCE = 0x01,
        KEY_EXCHANGE = 0x02,
        LEAVE = 0x03,
        MESSAGE = 0x04,
        FRAGMENT_START = 0x05,
        FRAGMENT_CONTINUE = 0x06,
        FRAGMENT_END = 0x07,
        CHANNEL_ANNOUNCE = 0x08,
        CHANNEL_RETENTION = 0x09,
        DELIVERY_ACK = 0x0A,
        DELIVERY_STATUS_REQUEST = 0x0B,
        READ_RECEIPT = 0x0C
    };
    
    // Special recipient IDs
    static const std::vector<uint8_t> BROADCAST_RECIPIENT = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static const std::vector<uint8_t> CHANNEL_RECIPIENT = {0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE};
}

// Message structure
struct BitchatMessage {
    std::string id;
    std::string sender;
    std::string senderPeerID;
    std::string content;
    QDateTime timestamp;
    std::string channel;
    bool isPrivateMessage;
    bool isRelay;
    std::string originalSender;
    std::string recipientNickname;
    std::vector<std::string> mentions;
    
    // Constructors
    BitchatMessage();
    BitchatMessage(const std::string& sender, const std::string& content, 
                   const QDateTime& timestamp = QDateTime::currentDateTime(),
                   bool isRelay = false, const std::string& originalSender = "",
                   bool isPrivateMessage = false, const std::string& recipientNickname = "",
                   const std::string& senderPeerID = "",
                   const std::vector<std::string>& mentions = {},
                   const std::string& channel = "");
    
    // Serialization
    std::vector<uint8_t> toBinaryPayload() const;
    static BitchatMessage fromBinaryPayload(const std::vector<uint8_t>& payload);
    
    // Getters
    std::string getId() const;
    std::string getSender() const;
    std::string getSenderPeerID() const;
    std::string getContent() const;
    QDateTime getTimestamp() const;
    std::string getChannel() const;
    bool isPrivate() const;
    std::vector<std::string> getMentions() const;
};

// Packet structure
struct BitchatPacket {
    uint8_t messageType;
    uint8_t ttl;
    std::vector<uint8_t> senderID;
    std::vector<uint8_t> recipientID;
    std::vector<uint8_t> payload;
    
    // Constructors
    BitchatPacket();
    BitchatPacket(uint8_t messageType, uint8_t ttl, 
                  const std::string& senderID, 
                  const std::vector<uint8_t>& payload);
    
    // Serialization
    std::vector<uint8_t> toBinaryData() const;
    static std::unique_ptr<BitchatPacket> fromBinaryData(const std::vector<uint8_t>& data);
    
    // Validation
    bool isValid() const;
    
    // Utility
    std::string getSenderIDString() const;
    std::string getRecipientIDString() const;
    void setSenderID(const std::string& senderID);
    void setRecipientID(const std::string& recipientID);
    
private:
    static std::vector<uint8_t> stringToBytes(const std::string& str, size_t maxSize);
    static std::string bytesToString(const std::vector<uint8_t>& bytes);
}; 