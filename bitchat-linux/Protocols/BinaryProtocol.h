#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include "../Utils/CompressionUtil.h"
#include "../Utils/MessagePadding.h"

// Message types matching iOS exactly
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

// Delivery status enum matching iOS
enum class DeliveryStatus {
    SENDING,
    SENT,
    DELIVERED,
    READ,
    FAILED,
    PARTIALLY_DELIVERED
};

// BitchatPacket structure matching iOS
struct BitchatPacket {
    uint8_t version;
    uint8_t type;
    std::vector<uint8_t> senderID;
    std::vector<uint8_t> recipientID;
    uint64_t timestamp;
    std::vector<uint8_t> payload;
    std::vector<uint8_t> signature;
    uint8_t ttl;
    
    BitchatPacket();
    BitchatPacket(uint8_t type, uint8_t ttl, const std::string& senderID, const std::vector<uint8_t>& payload);
    
    // Serialization
    std::vector<uint8_t> toBinaryData() const;
    static std::unique_ptr<BitchatPacket> fromBinaryData(const std::vector<uint8_t>& data);
};

// Enhanced BitchatMessage structure matching iOS
struct BitchatMessage {
    std::string id;
    std::string sender;
    std::string content;
    uint64_t timestamp;
    bool isRelay;
    std::string originalSender;
    bool isPrivate;
    std::string recipientNickname;
    std::string senderPeerID;
    std::vector<std::string> mentions;
    std::string channel;
    std::vector<uint8_t> encryptedContent;
    bool isEncrypted;
    DeliveryStatus deliveryStatus;
    
    BitchatMessage();
    BitchatMessage(const std::string& id, const std::string& sender, const std::string& content, 
                   uint64_t timestamp, bool isRelay = false, bool isPrivate = false);
    
    // Serialization for payload
    std::vector<uint8_t> toBinaryPayload() const;
    static std::unique_ptr<BitchatMessage> fromBinaryPayload(const std::vector<uint8_t>& payload);
};

// DeliveryAck structure matching iOS
struct DeliveryAck {
    std::string originalMessageID;
    std::string ackID;
    std::string recipientID;
    std::string recipientNickname;
    uint64_t timestamp;
    uint8_t hopCount;
    
    DeliveryAck(const std::string& originalMessageID, const std::string& recipientID, 
                const std::string& recipientNickname, uint8_t hopCount);
    
    std::vector<uint8_t> encode() const;
    static std::unique_ptr<DeliveryAck> decode(const std::vector<uint8_t>& data);
};

// ReadReceipt structure matching iOS
struct ReadReceipt {
    std::string originalMessageID;
    std::string receiptID;
    std::string readerID;
    std::string readerNickname;
    uint64_t timestamp;
    
    ReadReceipt(const std::string& originalMessageID, const std::string& readerID, 
                const std::string& readerNickname);
    
    std::vector<uint8_t> encode() const;
    static std::unique_ptr<ReadReceipt> decode(const std::vector<uint8_t>& data);
};

// BinaryProtocol class for encoding/decoding
class BinaryProtocol {
public:
    static const int HEADER_SIZE = 13;
    static const int SENDER_ID_SIZE = 8;
    static const int RECIPIENT_ID_SIZE = 8;
    static const int SIGNATURE_SIZE = 64;
    
    struct Flags {
        static const uint8_t HAS_RECIPIENT = 0x01;
        static const uint8_t HAS_SIGNATURE = 0x02;
        static const uint8_t IS_COMPRESSED = 0x04;
    };
    
    // Special recipient for broadcast messages
    static const std::vector<uint8_t> BROADCAST_RECIPIENT;
    
    static std::vector<uint8_t> encode(const BitchatPacket& packet);
    static std::unique_ptr<BitchatPacket> decode(const std::vector<uint8_t>& data);
    
private:
    static void writeUInt64BigEndian(std::vector<uint8_t>& data, uint64_t value);
    static uint64_t readUInt64BigEndian(const std::vector<uint8_t>& data, size_t offset);
    static void writeUInt16BigEndian(std::vector<uint8_t>& data, uint16_t value);
    static uint16_t readUInt16BigEndian(const std::vector<uint8_t>& data, size_t offset);
}; 