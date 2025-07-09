#include "BinaryProtocol.h"
#include <chrono>
#include <random>
#include <algorithm>
#include <cstring>
#include <zlib.h>
#include <QUuid>
#include <QDebug>

// Special recipient for broadcast messages
const std::vector<uint8_t> BinaryProtocol::BROADCAST_RECIPIENT = {
    0x42, 0x52, 0x4F, 0x41, 0x44, 0x43, 0x53, 0x54  // "BROADCST"
};

// BitchatPacket implementation
BitchatPacket::BitchatPacket() : version(1), type(0), ttl(0), timestamp(0) {}

BitchatPacket::BitchatPacket(uint8_t type, uint8_t ttl, const std::string& senderID, const std::vector<uint8_t>& payload)
    : version(1), type(type), ttl(ttl), payload(payload) {
    
    // Convert senderID to fixed 8-byte format
    this->senderID.resize(BinaryProtocol::SENDER_ID_SIZE, 0);
    std::vector<uint8_t> senderBytes(senderID.begin(), senderID.end());
    std::copy(senderBytes.begin(), 
              senderBytes.begin() + std::min(senderBytes.size(), (size_t)BinaryProtocol::SENDER_ID_SIZE),
              this->senderID.begin());
    
    // Set timestamp to current time in milliseconds
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    this->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::vector<uint8_t> BitchatPacket::toBinaryData() const {
    return BinaryProtocol::encode(*this);
}

std::unique_ptr<BitchatPacket> BitchatPacket::fromBinaryData(const std::vector<uint8_t>& data) {
    return BinaryProtocol::decode(data);
}

// BitchatMessage implementation
BitchatMessage::BitchatMessage() : timestamp(0), isRelay(false), isPrivate(false), isEncrypted(false), 
                                   deliveryStatus(DeliveryStatus::SENDING) {}

BitchatMessage::BitchatMessage(const std::string& id, const std::string& sender, const std::string& content, 
                               uint64_t timestamp, bool isRelay, bool isPrivate)
    : id(id), sender(sender), content(content), timestamp(timestamp), isRelay(isRelay), isPrivate(isPrivate),
      isEncrypted(false), deliveryStatus(DeliveryStatus::SENDING) {
    
    if (id.empty()) {
        this->id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    }
}

std::vector<uint8_t> BitchatMessage::toBinaryPayload() const {
    // Simple JSON-like serialization for compatibility
    std::string json = "{";
    json += "\"id\":\"" + id + "\",";
    json += "\"sender\":\"" + sender + "\",";
    json += "\"content\":\"" + content + "\",";
    json += "\"timestamp\":" + std::to_string(timestamp) + ",";
    json += "\"isRelay\":" + std::string(isRelay ? "true" : "false") + ",";
    json += "\"isPrivate\":" + std::string(isPrivate ? "true" : "false") + ",";
    json += "\"isEncrypted\":" + std::string(isEncrypted ? "true" : "false");
    
    if (!originalSender.empty()) {
        json += ",\"originalSender\":\"" + originalSender + "\"";
    }
    if (!recipientNickname.empty()) {
        json += ",\"recipientNickname\":\"" + recipientNickname + "\"";
    }
    if (!senderPeerID.empty()) {
        json += ",\"senderPeerID\":\"" + senderPeerID + "\"";
    }
    if (!channel.empty()) {
        json += ",\"channel\":\"" + channel + "\"";
    }
    if (!mentions.empty()) {
        json += ",\"mentions\":[";
        for (size_t i = 0; i < mentions.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + mentions[i] + "\"";
        }
        json += "]";
    }
    
    json += "}";
    
    return std::vector<uint8_t>(json.begin(), json.end());
}

std::unique_ptr<BitchatMessage> BitchatMessage::fromBinaryPayload(const std::vector<uint8_t>& payload) {
    // Simple JSON parsing - in production, use a proper JSON library
    std::string json(payload.begin(), payload.end());
    
    auto message = std::make_unique<BitchatMessage>();
    
    // Extract fields using simple string parsing
    auto extractString = [&json](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t start = json.find(search);
        if (start == std::string::npos) return "";
        start += search.length();
        size_t end = json.find("\"", start);
        if (end == std::string::npos) return "";
        return json.substr(start, end - start);
    };
    
    auto extractBool = [&json](const std::string& key) -> bool {
        std::string search = "\"" + key + "\":";
        size_t start = json.find(search);
        if (start == std::string::npos) return false;
        start += search.length();
        return json.substr(start, 4) == "true";
    };
    
    auto extractNumber = [&json](const std::string& key) -> uint64_t {
        std::string search = "\"" + key + "\":";
        size_t start = json.find(search);
        if (start == std::string::npos) return 0;
        start += search.length();
        size_t end = json.find_first_of(",}", start);
        if (end == std::string::npos) return 0;
        return std::stoull(json.substr(start, end - start));
    };
    
    message->id = extractString("id");
    message->sender = extractString("sender");
    message->content = extractString("content");
    message->timestamp = extractNumber("timestamp");
    message->isRelay = extractBool("isRelay");
    message->isPrivate = extractBool("isPrivate");
    message->isEncrypted = extractBool("isEncrypted");
    message->originalSender = extractString("originalSender");
    message->recipientNickname = extractString("recipientNickname");
    message->senderPeerID = extractString("senderPeerID");
    message->channel = extractString("channel");
    
    return message;
}

// DeliveryAck implementation
DeliveryAck::DeliveryAck(const std::string& originalMessageID, const std::string& recipientID, 
                         const std::string& recipientNickname, uint8_t hopCount)
    : originalMessageID(originalMessageID), recipientID(recipientID), 
      recipientNickname(recipientNickname), hopCount(hopCount) {
    
    this->ackID = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    this->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::vector<uint8_t> DeliveryAck::encode() const {
    std::string json = "{";
    json += "\"originalMessageID\":\"" + originalMessageID + "\",";
    json += "\"ackID\":\"" + ackID + "\",";
    json += "\"recipientID\":\"" + recipientID + "\",";
    json += "\"recipientNickname\":\"" + recipientNickname + "\",";
    json += "\"timestamp\":" + std::to_string(timestamp) + ",";
    json += "\"hopCount\":" + std::to_string(hopCount);
    json += "}";
    
    return std::vector<uint8_t>(json.begin(), json.end());
}

std::unique_ptr<DeliveryAck> DeliveryAck::decode(const std::vector<uint8_t>& data) {
    std::string json(data.begin(), data.end());
    
    // Simple JSON parsing
    auto extractString = [&json](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t start = json.find(search);
        if (start == std::string::npos) return "";
        start += search.length();
        size_t end = json.find("\"", start);
        if (end == std::string::npos) return "";
        return json.substr(start, end - start);
    };
    
    auto extractNumber = [&json](const std::string& key) -> uint64_t {
        std::string search = "\"" + key + "\":";
        size_t start = json.find(search);
        if (start == std::string::npos) return 0;
        start += search.length();
        size_t end = json.find_first_of(",}", start);
        if (end == std::string::npos) return 0;
        return std::stoull(json.substr(start, end - start));
    };
    
    auto ack = std::make_unique<DeliveryAck>("", "", "", 0);
    ack->originalMessageID = extractString("originalMessageID");
    ack->ackID = extractString("ackID");
    ack->recipientID = extractString("recipientID");
    ack->recipientNickname = extractString("recipientNickname");
    ack->timestamp = extractNumber("timestamp");
    ack->hopCount = (uint8_t)extractNumber("hopCount");
    
    return ack;
}

// ReadReceipt implementation
ReadReceipt::ReadReceipt(const std::string& originalMessageID, const std::string& readerID, 
                         const std::string& readerNickname)
    : originalMessageID(originalMessageID), readerID(readerID), readerNickname(readerNickname) {
    
    this->receiptID = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    this->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::vector<uint8_t> ReadReceipt::encode() const {
    std::string json = "{";
    json += "\"originalMessageID\":\"" + originalMessageID + "\",";
    json += "\"receiptID\":\"" + receiptID + "\",";
    json += "\"readerID\":\"" + readerID + "\",";
    json += "\"readerNickname\":\"" + readerNickname + "\",";
    json += "\"timestamp\":" + std::to_string(timestamp);
    json += "}";
    
    return std::vector<uint8_t>(json.begin(), json.end());
}

std::unique_ptr<ReadReceipt> ReadReceipt::decode(const std::vector<uint8_t>& data) {
    std::string json(data.begin(), data.end());
    
    // Simple JSON parsing
    auto extractString = [&json](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t start = json.find(search);
        if (start == std::string::npos) return "";
        start += search.length();
        size_t end = json.find("\"", start);
        if (end == std::string::npos) return "";
        return json.substr(start, end - start);
    };
    
    auto extractNumber = [&json](const std::string& key) -> uint64_t {
        std::string search = "\"" + key + "\":";
        size_t start = json.find(search);
        if (start == std::string::npos) return 0;
        start += search.length();
        size_t end = json.find_first_of(",}", start);
        if (end == std::string::npos) return 0;
        return std::stoull(json.substr(start, end - start));
    };
    
    auto receipt = std::make_unique<ReadReceipt>("", "", "");
    receipt->originalMessageID = extractString("originalMessageID");
    receipt->receiptID = extractString("receiptID");
    receipt->readerID = extractString("readerID");
    receipt->readerNickname = extractString("readerNickname");
    receipt->timestamp = extractNumber("timestamp");
    
    return receipt;
}

// BinaryProtocol implementation
std::vector<uint8_t> BinaryProtocol::encode(const BitchatPacket& packet) {
    std::vector<uint8_t> data;
    
    // Compress payload if beneficial
    std::vector<uint8_t> payload = packet.payload;
    std::vector<uint8_t> originalSizeBytes;
    bool isCompressed = false;
    
    if (CompressionUtil::shouldCompress(payload)) {
        std::vector<uint8_t> compressed = CompressionUtil::compress(payload);
        if (compressed.size() < payload.size()) {
            // Store original size (2 bytes)
            writeUInt16BigEndian(originalSizeBytes, (uint16_t)payload.size());
            payload = compressed;
            isCompressed = true;
        }
    }
    
    // Header (13 bytes)
    data.push_back(packet.version);
    data.push_back(packet.type);
    data.push_back(packet.ttl);
    
    // Timestamp (8 bytes, big-endian)
    writeUInt64BigEndian(data, packet.timestamp);
    
    // Flags (1 byte)
    uint8_t flags = 0;
    if (!packet.recipientID.empty()) {
        flags |= Flags::HAS_RECIPIENT;
    }
    if (!packet.signature.empty()) {
        flags |= Flags::HAS_SIGNATURE;
    }
    if (isCompressed) {
        flags |= Flags::IS_COMPRESSED;
    }
    data.push_back(flags);
    
    // Payload length (2 bytes, big-endian) - includes original size if compressed
    uint16_t payloadLength = (uint16_t)(payload.size() + (isCompressed ? 2 : 0));
    writeUInt16BigEndian(data, payloadLength);
    
    // SenderID (exactly 8 bytes)
    std::vector<uint8_t> senderBytes = packet.senderID;
    senderBytes.resize(SENDER_ID_SIZE, 0);
    data.insert(data.end(), senderBytes.begin(), senderBytes.end());
    
    // RecipientID (if present, exactly 8 bytes)
    if (!packet.recipientID.empty()) {
        std::vector<uint8_t> recipientBytes = packet.recipientID;
        recipientBytes.resize(RECIPIENT_ID_SIZE, 0);
        data.insert(data.end(), recipientBytes.begin(), recipientBytes.end());
    }
    
    // Payload (with original size prepended if compressed)
    if (isCompressed) {
        data.insert(data.end(), originalSizeBytes.begin(), originalSizeBytes.end());
    }
    data.insert(data.end(), payload.begin(), payload.end());
    
    // Signature (if present, exactly 64 bytes)
    if (!packet.signature.empty()) {
        std::vector<uint8_t> signatureBytes = packet.signature;
        signatureBytes.resize(SIGNATURE_SIZE, 0);
        data.insert(data.end(), signatureBytes.begin(), signatureBytes.end());
    }
    
    return data;
}

std::unique_ptr<BitchatPacket> BinaryProtocol::decode(const std::vector<uint8_t>& data) {
    if (data.size() < HEADER_SIZE + SENDER_ID_SIZE) {
        return nullptr;
    }
    
    size_t offset = 0;
    
    // Header
    uint8_t version = data[offset++];
    if (version != 1) return nullptr;  // Only support version 1
    
    uint8_t type = data[offset++];
    uint8_t ttl = data[offset++];
    
    // Timestamp
    uint64_t timestamp = readUInt64BigEndian(data, offset);
    offset += 8;
    
    // Flags
    uint8_t flags = data[offset++];
    bool hasRecipient = (flags & Flags::HAS_RECIPIENT) != 0;
    bool hasSignature = (flags & Flags::HAS_SIGNATURE) != 0;
    bool isCompressed = (flags & Flags::IS_COMPRESSED) != 0;
    
    // Payload length
    uint16_t payloadLength = readUInt16BigEndian(data, offset);
    offset += 2;
    
    // Calculate expected total size
    size_t expectedSize = HEADER_SIZE + SENDER_ID_SIZE + payloadLength;
    if (hasRecipient) {
        expectedSize += RECIPIENT_ID_SIZE;
    }
    if (hasSignature) {
        expectedSize += SIGNATURE_SIZE;
    }
    
    if (data.size() < expectedSize) {
        return nullptr;
    }
    
    // SenderID
    std::vector<uint8_t> senderID(data.begin() + offset, data.begin() + offset + SENDER_ID_SIZE);
    offset += SENDER_ID_SIZE;
    
    // RecipientID
    std::vector<uint8_t> recipientID;
    if (hasRecipient) {
        recipientID = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + RECIPIENT_ID_SIZE);
        offset += RECIPIENT_ID_SIZE;
    }
    
    // Payload
    std::vector<uint8_t> payload;
    if (isCompressed) {
        if (payloadLength < 2) return nullptr;
        
        uint16_t originalSize = readUInt16BigEndian(data, offset);
        offset += 2;
        
        std::vector<uint8_t> compressedPayload(data.begin() + offset, data.begin() + offset + payloadLength - 2);
        offset += payloadLength - 2;
        
        payload = CompressionUtil::decompress(compressedPayload);
        if (payload.empty()) return nullptr;
    } else {
        payload = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + payloadLength);
        offset += payloadLength;
    }
    
    // Signature
    std::vector<uint8_t> signature;
    if (hasSignature) {
        signature = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + SIGNATURE_SIZE);
        offset += SIGNATURE_SIZE;
    }
    
    auto packet = std::make_unique<BitchatPacket>();
    packet->version = version;
    packet->type = type;
    packet->ttl = ttl;
    packet->timestamp = timestamp;
    packet->senderID = senderID;
    packet->recipientID = recipientID;
    packet->payload = payload;
    packet->signature = signature;
    
    return packet;
}

// Helper functions for big-endian encoding/decoding
void BinaryProtocol::writeUInt64BigEndian(std::vector<uint8_t>& data, uint64_t value) {
    for (int i = 7; i >= 0; --i) {
        data.push_back((value >> (i * 8)) & 0xFF);
    }
}

uint64_t BinaryProtocol::readUInt64BigEndian(const std::vector<uint8_t>& data, size_t offset) {
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | data[offset + i];
    }
    return value;
}

void BinaryProtocol::writeUInt16BigEndian(std::vector<uint8_t>& data, uint16_t value) {
    data.push_back((value >> 8) & 0xFF);
    data.push_back(value & 0xFF);
}

uint16_t BinaryProtocol::readUInt16BigEndian(const std::vector<uint8_t>& data, size_t offset) {
    return ((uint16_t)data[offset] << 8) | data[offset + 1];
} 