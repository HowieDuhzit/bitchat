#pragma once

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QBluetoothServer>
#include <QBluetoothSocket>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothServiceInfo>
#include <QBluetoothUuid>
#include <QTimer>
#include <QMap>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include "../Utils/OptimizedBloomFilter.h"
#include "../Protocols/BinaryProtocol.h"

class MessageRetryService;
class DeliveryTracker;
class MessageRetentionService;
class KeychainManager;
struct BitchatMessage;
struct BitchatPacket;

struct ConnectedPeer {
    std::string id;
    std::string nickname;
    QBluetoothAddress address;
    QBluetoothSocket* socket;
    bool isConnected;
    uint64_t lastSeen;
    int rssi;
    std::vector<uint8_t> receiveBuffer;
    
    ConnectedPeer() : socket(nullptr), isConnected(false), lastSeen(0), rssi(0) {}
};

class BluetoothMeshService : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothMeshService(QObject* parent = nullptr);
    ~BluetoothMeshService();
    
    // Service lifecycle
    bool initialize();
    void start();
    void stop();
    
    // Message sending
    void sendMessage(const std::string& content, const std::string& channel = "");
    void sendPrivateMessage(const std::string& content, const std::string& peerId);
    void announceNickname(const std::string& nickname);
    void sendBroadcast(const std::vector<uint8_t>& data);
    
    // Peer management
    std::string getMyPeerId() const;
    std::vector<ConnectedPeer> getConnectedPeers() const;
    
    // Status
    bool isRunning() const;
    bool isConnected() const;
    int getConnectedPeerCount() const;
    
    // Configuration
    void setAdvertisementInterval(int intervalMs);
    void setScanInterval(int intervalMs);
    void setMaxConnections(int maxConnections);
    void setTransmissionPower(int powerLevel);

signals:
    void messageReceived(const std::vector<uint8_t>& data, const std::string& peerId);
    void peerConnected(const std::string& peerId, const std::string& nickname);
    void peerDisconnected(const std::string& peerId);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString& error);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo& device);
    void onDiscoveryFinished();
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onNewConnection();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QBluetoothSocket::SocketError error);
    void onSocketReadyRead();
    void onAdvertisementTimeout();
    void onScanTimeout();
    void onPeerCleanupTimeout();
    void onHeartbeatTimeout();
    void handleRetryRequest(const QString& messageID);
    void handleDeliveryStatusUpdate(const QString& messageID, DeliveryStatus status);

private:
    // Bluetooth components
    QBluetoothLocalDevice* m_localDevice;
    QBluetoothDeviceDiscoveryAgent* m_discoveryAgent;
    QBluetoothServer* m_server;
    QBluetoothServiceInfo m_serviceInfo;
    QBluetoothSocket* m_socket;
    
    // Service UUIDs - matching iOS exactly
    static const QBluetoothUuid BITCHAT_SERVICE_UUID;
    static const QBluetoothUuid BITCHAT_CHARACTERISTIC_UUID;
    
    // Peer management
    std::unordered_map<std::string, std::unique_ptr<ConnectedPeer>> m_peers;
    std::unordered_map<QBluetoothSocket*, std::string> m_socketToPeerId;
    
    // State
    bool m_isRunning = false;
    bool m_isAdvertising = false;
    bool m_isScanning = false;
    std::string m_myPeerId;
    std::string m_myNickname;
    
    // Timers
    QTimer* m_advertisementTimer;
    QTimer* m_scanTimer;
    QTimer* m_peerCleanupTimer;
    QTimer* m_heartbeatTimer;
    QTimer* m_reconnectTimer;
    
    // Configuration
    int m_advertisementInterval = 5000;  // 5 seconds
    int m_scanInterval = 10000;          // 10 seconds
    int m_maxConnections = 8;
    int m_transmissionPower = 0;
    
    // Constants
    static const int PEER_TIMEOUT = 30000;      // 30 seconds
    static const int HEARTBEAT_INTERVAL = 10000; // 10 seconds
    static const uint8_t MESSAGE_TYPE_CHAT = 0x01;
    static const uint8_t MESSAGE_TYPE_PRIVATE = 0x02;
    static const uint8_t MESSAGE_TYPE_CHANNEL = 0x03;
    static const uint8_t MESSAGE_TYPE_NICKNAME = 0x04;
    static const uint8_t MESSAGE_TYPE_HEARTBEAT = 0x05;
    static const uint8_t MESSAGE_TYPE_RELAY = 0x06;
    static const uint8_t MESSAGE_TYPE_PEER_INFO_REQUEST = 0x07;
    
    // Services
    MessageRetryService* m_retryService;
    DeliveryTracker* m_deliveryTracker;
    MessageRetentionService* m_retentionService;
    KeychainManager* m_keychain;
    OptimizedBloomFilter* m_messageFilter;
    
    // Helper methods
    void setupService();
    void startAdvertising();
    void stopAdvertising();
    void startScanning();
    void stopScanning();
    void connectToPeer(const QBluetoothDeviceInfo& device);
    void relayMessage(const std::vector<uint8_t>& data, const std::string& excludePeerId);
    void sendToPeer(const std::string& peerId, const std::vector<uint8_t>& data);
    void sendToAllPeers(const std::vector<uint8_t>& data, const std::string& excludePeerId = "");
    void processIncomingData(const std::string& peerId, std::vector<uint8_t>& buffer);
    void handleIncomingMessage(const std::string& peerId, const std::vector<uint8_t>& data);
    void handleNicknameAnnouncement(const std::string& peerId, const std::vector<uint8_t>& data);
    void handleHeartbeat(const std::string& peerId, const std::vector<uint8_t>& data);
    void sendHeartbeat();
    void updatePeerLastSeen(const std::string& peerId);
    void checkPeerTimeouts();
    void disconnectPeer(const std::string& peerId);
    void cleanupPeer(const std::string& peerId);
    void requestPeerInfo(const std::string& peerId);
    
    // Utility methods
    std::string generatePeerId() const;
    uint64_t getCurrentTimestamp() const;
    bool isValidPeerId(const std::string& peerId) const;
    uint8_t calculateChecksum(const std::vector<uint8_t>& data) const;
    bool verifyChecksum(const std::vector<uint8_t>& data) const;
    std::vector<uint8_t> addProtocolHeader(const std::vector<uint8_t>& payload, uint8_t messageType) const;
    std::vector<uint8_t> removeProtocolHeader(const std::vector<uint8_t>& packet, uint8_t& messageType) const;
    std::vector<uint8_t> createMessagePacket(const std::string& content, const std::string& channel = "", const std::string& recipientId = "");
    std::vector<uint8_t> createHeartbeatPacket();
    std::vector<uint8_t> createNicknamePacket(const std::string& nickname);
}; 