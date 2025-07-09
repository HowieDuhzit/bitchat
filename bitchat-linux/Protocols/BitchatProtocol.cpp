#include "BitchatProtocol.h"
#include <QUuid>
#include <QDataStream>
#include <QByteArray>
#include <QIODevice>
#include <QDebug>
#include <algorithm>
#include <cstring>

// BitchatMessage implementation
BitchatMessage::BitchatMessage()
    : timestamp(QDateTime::currentDateTime())
    , isPrivateMessage(false)
    , isRelay(false)
{
    id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
}

BitchatMessage::BitchatMessage(const std::string& sender, const std::string& content, 
                               const QDateTime& timestamp, bool isRelay, 
                               const std::string& originalSender, bool isPrivateMessage,
                               const std::string& recipientNickname, 
                               const std::string& senderPeerID,
                               const std::vector<std::string>& mentions,
                               const std::string& channel)
    : sender(sender)
    , content(content)
    , timestamp(timestamp)
    , isRelay(isRelay)
    , originalSender(originalSender)
    , isPrivateMessage(isPrivateMessage)
    , recipientNickname(recipientNickname)
    , senderPeerID(senderPeerID)
    , mentions(mentions)
    , channel(channel)
{
    id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
}

std::vector<uint8_t> BitchatMessage::toBinaryPayload() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Write message fields
    stream << QString::fromStdString(id);
    stream << QString::fromStdString(sender);
    stream << QString::fromStdString(senderPeerID);
    stream << QString::fromStdString(content);
    stream << timestamp;
    stream << QString::fromStdString(channel);
    stream << isPrivateMessage;
    stream << isRelay;
    stream << QString::fromStdString(originalSender);
    stream << QString::fromStdString(recipientNickname);
    
    // Write mentions
    stream << static_cast<quint32>(mentions.size());
    for (const auto& mention : mentions) {
        stream << QString::fromStdString(mention);
    }
    
    return std::vector<uint8_t>(data.begin(), data.end());
}

BitchatMessage BitchatMessage::fromBinaryPayload(const std::vector<uint8_t>& payload)
{
    QByteArray data(reinterpret_cast<const char*>(payload.data()), payload.size());
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    BitchatMessage message;
    QString qstr;
    
    // Read message fields
    stream >> qstr; message.id = qstr.toStdString();
    stream >> qstr; message.sender = qstr.toStdString();
    stream >> qstr; message.senderPeerID = qstr.toStdString();
    stream >> qstr; message.content = qstr.toStdString();
    stream >> message.timestamp;
    stream >> qstr; message.channel = qstr.toStdString();
    stream >> message.isPrivateMessage;
    stream >> message.isRelay;
    stream >> qstr; message.originalSender = qstr.toStdString();
    stream >> qstr; message.recipientNickname = qstr.toStdString();
    
    // Read mentions
    quint32 mentionCount;
    stream >> mentionCount;
    message.mentions.reserve(mentionCount);
    for (quint32 i = 0; i < mentionCount; ++i) {
        stream >> qstr;
        message.mentions.push_back(qstr.toStdString());
    }
    
    return message;
}

std::string BitchatMessage::getId() const { return id; }
std::string BitchatMessage::getSender() const { return sender; }
std::string BitchatMessage::getSenderPeerID() const { return senderPeerID; }
std::string BitchatMessage::getContent() const { return content; }
QDateTime BitchatMessage::getTimestamp() const { return timestamp; }
std::string BitchatMessage::getChannel() const { return channel; }
bool BitchatMessage::isPrivate() const { return isPrivateMessage; }
std::vector<std::string> BitchatMessage::getMentions() const { return mentions; }

// BitchatPacket implementation
BitchatPacket::BitchatPacket()
    : messageType(0)
    , ttl(BitchatProtocol::MAX_TTL)
    , senderID(BitchatProtocol::SENDER_ID_SIZE, 0)
    , recipientID(BitchatProtocol::BROADCAST_RECIPIENT)
{
}

BitchatPacket::BitchatPacket(uint8_t messageType, uint8_t ttl, 
                             const std::string& senderID, 
                             const std::vector<uint8_t>& payload)
    : messageType(messageType)
    , ttl(ttl)
    , payload(payload)
    , recipientID(BitchatProtocol::BROADCAST_RECIPIENT)
{
    setSenderID(senderID);
}

std::vector<uint8_t> BitchatPacket::toBinaryData() const
{
    std::vector<uint8_t> data;
    data.reserve(BitchatProtocol::HEADER_SIZE + payload.size());
    
    // Protocol version
    data.push_back(BitchatProtocol::PROTOCOL_VERSION);
    
    // Message type
    data.push_back(messageType);
    
    // TTL
    data.push_back(ttl);
    
    // Sender ID (8 bytes)
    data.insert(data.end(), senderID.begin(), senderID.end());
    
    // Recipient ID (8 bytes) - not included in header size calculation
    data.insert(data.end(), recipientID.begin(), recipientID.end());
    
    // Payload
    data.insert(data.end(), payload.begin(), payload.end());
    
    return data;
}

std::unique_ptr<BitchatPacket> BitchatPacket::fromBinaryData(const std::vector<uint8_t>& data)
{
    if (data.size() < BitchatProtocol::HEADER_SIZE + BitchatProtocol::SENDER_ID_SIZE) {
        return nullptr;
    }
    
    size_t offset = 0;
    
    // Check protocol version
    if (data[offset++] != BitchatProtocol::PROTOCOL_VERSION) {
        return nullptr;
    }
    
    auto packet = std::make_unique<BitchatPacket>();
    
    // Message type
    packet->messageType = data[offset++];
    
    // TTL
    packet->ttl = data[offset++];
    
    // Sender ID (8 bytes)
    packet->senderID.assign(data.begin() + offset, data.begin() + offset + BitchatProtocol::SENDER_ID_SIZE);
    offset += BitchatProtocol::SENDER_ID_SIZE;
    
    // Recipient ID (8 bytes)
    packet->recipientID.assign(data.begin() + offset, data.begin() + offset + BitchatProtocol::SENDER_ID_SIZE);
    offset += BitchatProtocol::SENDER_ID_SIZE;
    
    // Payload
    packet->payload.assign(data.begin() + offset, data.end());
    
    return packet;
}

bool BitchatPacket::isValid() const
{
    return messageType > 0 && 
           ttl > 0 && 
           senderID.size() == BitchatProtocol::SENDER_ID_SIZE &&
           recipientID.size() == BitchatProtocol::SENDER_ID_SIZE &&
           payload.size() <= BitchatProtocol::MAX_PAYLOAD_SIZE;
}

std::string BitchatPacket::getSenderIDString() const
{
    return bytesToString(senderID);
}

std::string BitchatPacket::getRecipientIDString() const
{
    return bytesToString(recipientID);
}

void BitchatPacket::setSenderID(const std::string& senderID)
{
    this->senderID = stringToBytes(senderID, BitchatProtocol::SENDER_ID_SIZE);
}

void BitchatPacket::setRecipientID(const std::string& recipientID)
{
    this->recipientID = stringToBytes(recipientID, BitchatProtocol::SENDER_ID_SIZE);
}

std::vector<uint8_t> BitchatPacket::stringToBytes(const std::string& str, size_t maxSize)
{
    std::vector<uint8_t> bytes(maxSize, 0);
    size_t copySize = std::min(str.size(), maxSize);
    std::memcpy(bytes.data(), str.data(), copySize);
    return bytes;
}

std::string BitchatPacket::bytesToString(const std::vector<uint8_t>& bytes)
{
    // Find the end of the string (null terminator or end of vector)
    auto end = std::find(bytes.begin(), bytes.end(), 0);
    return std::string(bytes.begin(), end);
} 