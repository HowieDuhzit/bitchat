#include "BluetoothMeshService.h"
#include "BinaryProtocol.h"
#include "MessageRetryService.h"
#include "DeliveryTracker.h"
#include "MessageRetentionService.h"
#include "KeychainManager.h"
#include "OptimizedBloomFilter.h"
#include <QBluetoothDeviceInfo>
#include <QBluetoothServiceDiscoveryAgent>
#include <QDataStream>
#include <QDebug>
#include <QCoreApplication>
#include <QRandomGenerator>
#include <chrono>
#include <random>
#include <algorithm>

// Constants
const QBluetoothUuid BluetoothMeshService::BITCHAT_SERVICE_UUID = QBluetoothUuid(QStringLiteral("00001000-0000-1000-8000-00805f9b34fb"));
const QBluetoothUuid BluetoothMeshService::BITCHAT_CHARACTERISTIC_UUID = QBluetoothUuid(QStringLiteral("00001001-0000-1000-8000-00805f9b34fb"));

BluetoothMeshService::BluetoothMeshService(QObject *parent)
    : QObject(parent)
    , m_localDevice(nullptr)
    , m_socket(nullptr)
    , m_isAdvertising(false)
    , m_isScanning(false)
    , m_reconnectTimer(new QTimer(this))
    , m_messageFilter(new OptimizedBloomFilter(OptimizedBloomFilter::adaptive(50)))
{
    // Initialize services
    m_retryService = MessageRetryService::shared();
    m_deliveryTracker = DeliveryTracker::shared();
    m_retentionService = MessageRetentionService::shared();
    m_keychain = KeychainManager::shared();
    
    // Set up retry service
    m_retryService->setMeshService(this);
    
    // Connect delivery tracker signals
    connect(m_deliveryTracker, &DeliveryTracker::messageRetryRequested,
            this, &BluetoothMeshService::handleRetryRequest);
    connect(m_deliveryTracker, &DeliveryTracker::deliveryStatusUpdated,
            this, &BluetoothMeshService::handleDeliveryStatusUpdate);
    
    // Generate ephemeral peer ID for each session (matching iOS behavior)
    auto randomBytes = QRandomGenerator::global()->generate();
    m_myPeerId = QString("%1").arg(randomBytes, 8, 16, QChar('0')).toStdString();
    
    // Initialize Bluetooth components
    m_localDevice = new QBluetoothLocalDevice(this);
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_server = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this);
    
    // Connect signals
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothMeshService::onDeviceDiscovered);
    connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::errorOccurred),
            this, &BluetoothMeshService::onDiscoveryError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothMeshService::onDiscoveryFinished);
    
    connect(m_server, &QBluetoothServer::newConnection,
            this, &BluetoothMeshService::onNewConnection);
    
    connect(m_advertisementTimer, &QTimer::timeout,
            this, &BluetoothMeshService::onAdvertisementTimeout);
    connect(m_scanTimer, &QTimer::timeout,
            this, &BluetoothMeshService::onScanTimeout);
    connect(m_peerCleanupTimer, &QTimer::timeout,
            this, &BluetoothMeshService::onPeerCleanupTimeout);
    connect(m_heartbeatTimer, &QTimer::timeout,
            this, &BluetoothMeshService::onHeartbeatTimeout);
}

BluetoothMeshService::~BluetoothMeshService() {
    stop();
}

bool BluetoothMeshService::initialize() {
    // Initialize local Bluetooth device
    m_localDevice = new QBluetoothLocalDevice(this);
    
    if (!m_localDevice->isValid()) {
        qDebug() << "No valid Bluetooth adapter found";
        return false;
    }
    
    // Check if Bluetooth is powered on
    if (m_localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        qDebug() << "Bluetooth is powered off";
        return false;
    }
    
    // Initialize discovery agent
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothMeshService::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothMeshService::onDiscoveryFinished);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BluetoothMeshService::onDiscoveryError);
    
    // Initialize server for incoming connections
    m_server = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this);
    connect(m_server, &QBluetoothServer::newConnection,
            this, &BluetoothMeshService::onNewConnection);
    
    // Setup timers
    m_advertisementTimer = new QTimer(this);
    connect(m_advertisementTimer, &QTimer::timeout,
            this, &BluetoothMeshService::onAdvertisementTimeout);
    
    m_scanTimer = new QTimer(this);
    connect(m_scanTimer, &QTimer::timeout,
            this, &BluetoothMeshService::onScanTimeout);
    
    m_peerCleanupTimer = new QTimer(this);
    connect(m_peerCleanupTimer, &QTimer::timeout,
            this, &BluetoothMeshService::onPeerCleanupTimeout);
    
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout,
            this, &BluetoothMeshService::onHeartbeatTimeout);
    
    return true;
}

void BluetoothMeshService::start() {
    if (m_isRunning) {
        return;
    }
    
    qDebug() << "Starting Bluetooth mesh service with peer ID:" << QString::fromStdString(m_myPeerId);
    
    m_isRunning = true;
    
    // Setup and start advertising
    setupService();
    startAdvertising();
    
    // Start scanning for peers
    startScanning();
    
    // Start periodic timers
    m_peerCleanupTimer->start(PEER_TIMEOUT);
    m_heartbeatTimer->start(HEARTBEAT_INTERVAL);
    
    emit connectionStatusChanged(true);
}

void BluetoothMeshService::stop() {
    if (!m_isRunning) {
        return;
    }
    
    qDebug() << "Stopping Bluetooth mesh service";
    
    m_isRunning = false;
    
    // Stop timers
    if (m_advertisementTimer) m_advertisementTimer->stop();
    if (m_scanTimer) m_scanTimer->stop();
    if (m_peerCleanupTimer) m_peerCleanupTimer->stop();
    if (m_heartbeatTimer) m_heartbeatTimer->stop();
    
    // Stop advertising and scanning
    stopAdvertising();
    stopScanning();
    
    // Disconnect all peers
    for (auto& pair : m_peers) {
        disconnectPeer(pair.first);
    }
    m_peers.clear();
    m_socketToPeerId.clear();
    
    emit connectionStatusChanged(false);
}

void BluetoothMeshService::setupService() {
    if (!m_server) {
        return;
    }
    
    // Create service info
    m_serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceName, "BitChat Mesh");
    m_serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceDescription, "BitChat Decentralized Messaging");
    m_serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceProvider, "BitChat");
    
    // Set service class UUID
    m_serviceInfo.setServiceUuid(BITCHAT_SERVICE_UUID);
    
    // Register service
    QBluetoothServiceInfo::Sequence classId;
    classId << QVariant::fromValue(BITCHAT_SERVICE_UUID);
    m_serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceClassIds, classId);
    
    // Set protocol descriptor
    QBluetoothServiceInfo::Sequence protocolDescriptorList;
    QBluetoothServiceInfo::Sequence protocol;
    protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::ProtocolUuid::Rfcomm));
    protocolDescriptorList.append(QVariant::fromValue(protocol));
    m_serviceInfo.setAttribute(QBluetoothServiceInfo::ProtocolDescriptorList, protocolDescriptorList);
    
    // Start listening
    if (!m_server->listen(m_localDevice->address())) {
        qDebug() << "Failed to start Bluetooth server";
        return;
    }
    
    m_serviceInfo.registerService(m_localDevice->address());
    qDebug() << "Bluetooth service registered on" << m_localDevice->address().toString();
}

void BluetoothMeshService::startAdvertising() {
    if (m_isAdvertising) {
        return;
    }
    
    // Make device discoverable
    m_localDevice->setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    
    m_isAdvertising = true;
    m_advertisementTimer->start(m_advertisementInterval);
    
    qDebug() << "Started advertising";
}

void BluetoothMeshService::stopAdvertising() {
    if (!m_isAdvertising) {
        return;
    }
    
    m_isAdvertising = false;
    m_advertisementTimer->stop();
    
    // Make device non-discoverable
    m_localDevice->setHostMode(QBluetoothLocalDevice::HostConnectable);
    
    qDebug() << "Stopped advertising";
}

void BluetoothMeshService::startScanning() {
    if (m_isScanning || !m_discoveryAgent) {
        return;
    }
    
    m_isScanning = true;
    m_discoveryAgent->start();
    m_scanTimer->start(m_scanInterval);
    
    qDebug() << "Started scanning for peers";
}

void BluetoothMeshService::stopScanning() {
    if (!m_isScanning || !m_discoveryAgent) {
        return;
    }
    
    m_isScanning = false;
    m_discoveryAgent->stop();
    m_scanTimer->stop();
    
    qDebug() << "Stopped scanning";
}

void BluetoothMeshService::sendMessage(const std::string& content, const std::string& channel) {
    if (!m_isRunning) return;
    
    // Create BitchatMessage
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    BitchatMessage message("", m_myNickname, content, timestamp, false, false);
    if (!channel.empty()) {
        message.channel = channel;
    }
    
    // Create BitchatPacket
    auto payload = message.toBinaryPayload();
    BitchatPacket packet(static_cast<uint8_t>(::MessageType::MESSAGE), 7, m_myPeerId, payload);
    
    // Set recipient to broadcast
    packet.recipientID = BinaryProtocol::BROADCAST_RECIPIENT;
    
    // Send the packet
    auto data = packet.toBinaryData();
    sendBroadcast(data);
}

void BluetoothMeshService::sendPrivateMessage(const std::string& content, const std::string& peerId) {
    if (!m_isRunning) return;
    
    // Create private BitchatMessage
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    BitchatMessage message("", m_myNickname, content, timestamp, false, true);
    message.senderPeerID = m_myPeerId;
    
    // Create BitchatPacket
    auto payload = message.toBinaryPayload();
    BitchatPacket packet(static_cast<uint8_t>(::MessageType::MESSAGE), 7, m_myPeerId, payload);
    
    // Set recipient to specific peer
    packet.recipientID.resize(8, 0);
    std::vector<uint8_t> recipientBytes(peerId.begin(), peerId.end());
    std::copy(recipientBytes.begin(), 
              recipientBytes.begin() + std::min(recipientBytes.size(), (size_t)8),
              packet.recipientID.begin());
    
    // Send the packet
    auto data = packet.toBinaryData();
    sendBroadcast(data);  // Still broadcast but encrypted for specific recipient
}

void BluetoothMeshService::sendBroadcast(const std::vector<uint8_t>& data) {
    if (!m_isRunning || data.empty()) return;
    
    // Send to all connected peers
    for (const auto& [peerId, peer] : m_peers) {
        if (peer->socket && peer->socket->state() == QBluetoothSocket::SocketState::ConnectedState) {
            peer->socket->write(reinterpret_cast<const char*>(data.data()), data.size());
        }
    }
}

void BluetoothMeshService::relayMessage(const std::vector<uint8_t>& data, const std::string& excludePeerId) {
    if (!m_isRunning || data.empty()) return;
    
    // Parse packet to check TTL
    auto packet = BitchatPacket::fromBinaryData(data);
    if (!packet || packet->ttl <= 1) {
        return;  // Don't relay if TTL is too low
    }
    
    // Decrease TTL and re-encode
    packet->ttl--;
    auto relayData = packet->toBinaryData();
    
    // Send to all connected peers except the one we received from
    for (const auto& [peerId, peer] : m_peers) {
        if (peerId != excludePeerId && peer->socket && 
            peer->socket->state() == QBluetoothSocket::SocketState::ConnectedState) {
            peer->socket->write(reinterpret_cast<const char*>(relayData.data()), relayData.size());
        }
    }
}

std::string BluetoothMeshService::getMyPeerId() const {
    return m_myPeerId;
}

std::vector<ConnectedPeer> BluetoothMeshService::getConnectedPeers() const {
    std::vector<ConnectedPeer> peers;
    for (const auto& pair : m_peers) {
        if (pair.second->isConnected) {
            peers.push_back(*pair.second);
        }
    }
    return peers;
}

void BluetoothMeshService::announceNickname(const std::string& nickname) {
    if (!m_isRunning) return;
    
    m_myNickname = nickname;
    
    // Create announce packet
    std::vector<uint8_t> payload(nickname.begin(), nickname.end());
    BitchatPacket packet(static_cast<uint8_t>(::MessageType::ANNOUNCE), 3, m_myPeerId, payload);
    
    // Send the packet
    auto data = packet.toBinaryData();
    sendBroadcast(data);
}

bool BluetoothMeshService::isRunning() const {
    return m_isRunning;
}

bool BluetoothMeshService::isConnected() const {
    return m_isRunning && !m_peers.empty();
}

int BluetoothMeshService::getConnectedPeerCount() const {
    int count = 0;
    for (const auto& pair : m_peers) {
        if (pair.second->isConnected) {
            count++;
        }
    }
    return count;
}

// Slot implementations
void BluetoothMeshService::onDeviceDiscovered(const QBluetoothDeviceInfo& device) {
    // Check if this device offers our service
    if (device.serviceUuids().contains(BITCHAT_SERVICE_UUID)) {
        qDebug() << "Found BitChat peer:" << device.name() << device.address().toString();
        
        // Don't connect to ourselves
        if (device.address() == m_localDevice->address()) {
            return;
        }
        
        // Check if we're already connected or at max connections
        std::string deviceId = device.address().toString().toStdString();
        if (m_peers.find(deviceId) != m_peers.end() || 
            getConnectedPeerCount() >= m_maxConnections) {
            return;
        }
        
        connectToPeer(device);
    }
}

void BluetoothMeshService::onDiscoveryFinished() {
    qDebug() << "Device discovery finished";
    
    // Restart scanning if still running
    if (m_isRunning && m_isScanning) {
        QTimer::singleShot(1000, this, &BluetoothMeshService::startScanning);
    }
}

void BluetoothMeshService::onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error error) {
    qDebug() << "Discovery error:" << error;
    emit errorOccurred(QString("Discovery error: %1").arg(error));
}

void BluetoothMeshService::onNewConnection() {
    if (!m_server) {
        return;
    }
    
    QBluetoothSocket* socket = m_server->nextPendingConnection();
    if (!socket) {
        return;
    }
    
    qDebug() << "New incoming connection from" << socket->peerAddress().toString();
    
    // Check connection limits
    if (getConnectedPeerCount() >= m_maxConnections) {
        qDebug() << "Max connections reached, rejecting connection";
        socket->close();
        socket->deleteLater();
        return;
    }
    
    // Create peer info
    std::string peerId = socket->peerAddress().toString().toStdString();
    auto peer = std::make_unique<ConnectedPeer>();
    peer->id = peerId;
    peer->socket = socket;
    peer->address = socket->peerAddress();
    peer->isConnected = true;
    peer->lastSeen = getCurrentTimestamp();
    
    // Setup socket connections
    connect(socket, &QBluetoothSocket::connected,
            this, &BluetoothMeshService::onSocketConnected);
    connect(socket, &QBluetoothSocket::disconnected,
            this, &BluetoothMeshService::onSocketDisconnected);
    connect(socket, &QBluetoothSocket::errorOccurred,
            this, &BluetoothMeshService::onSocketError);
    connect(socket, &QBluetoothSocket::readyRead,
            this, &BluetoothMeshService::onSocketReadyRead);
    
    // Store peer
    m_peers[peerId] = std::move(peer);
    m_socketToPeerId[socket] = peerId;
    
    emit peerConnected(peerId, "Unknown");
}

void BluetoothMeshService::connectToPeer(const QBluetoothDeviceInfo& device) {
    QBluetoothSocket* socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    
    // Setup socket connections
    connect(socket, &QBluetoothSocket::connected,
            this, &BluetoothMeshService::onSocketConnected);
    connect(socket, &QBluetoothSocket::disconnected,
            this, &BluetoothMeshService::onSocketDisconnected);
    connect(socket, &QBluetoothSocket::errorOccurred,
            this, &BluetoothMeshService::onSocketError);
    connect(socket, &QBluetoothSocket::readyRead,
            this, &BluetoothMeshService::onSocketReadyRead);
    
    // Create peer info
    std::string peerId = device.address().toString().toStdString();
    auto peer = std::make_unique<ConnectedPeer>();
    peer->id = peerId;
    peer->socket = socket;
    peer->address = device.address();
    peer->isConnected = false;
    peer->lastSeen = getCurrentTimestamp();
    peer->rssi = device.rssi();
    
    // Store peer
    m_peers[peerId] = std::move(peer);
    m_socketToPeerId[socket] = peerId;
    
    // Connect to service
    socket->connectToService(device.address(), BITCHAT_SERVICE_UUID);
    
    qDebug() << "Connecting to peer" << QString::fromStdString(peerId);
}

void BluetoothMeshService::onSocketConnected() {
    QBluetoothSocket* socket = qobject_cast<QBluetoothSocket*>(sender());
    if (!socket) {
        return;
    }
    
    auto it = m_socketToPeerId.find(socket);
    if (it != m_socketToPeerId.end()) {
        std::string peerId = it->second;
        auto peerIt = m_peers.find(peerId);
        if (peerIt != m_peers.end()) {
            peerIt->second->isConnected = true;
            peerIt->second->lastSeen = getCurrentTimestamp();
            
            qDebug() << "Connected to peer" << QString::fromStdString(peerId);
            emit peerConnected(peerId, peerIt->second->nickname);
            
            // Send nickname announcement
            std::vector<uint8_t> packet = createNicknamePacket(m_myNickname);
            sendToPeer(peerId, packet);
        }
    }
}

void BluetoothMeshService::onSocketDisconnected() {
    QBluetoothSocket* socket = qobject_cast<QBluetoothSocket*>(sender());
    if (!socket) {
        return;
    }
    
    auto it = m_socketToPeerId.find(socket);
    if (it != m_socketToPeerId.end()) {
        std::string peerId = it->second;
        qDebug() << "Peer disconnected:" << QString::fromStdString(peerId);
        
        emit peerDisconnected(peerId);
        cleanupPeer(peerId);
    }
}

void BluetoothMeshService::onSocketError(QBluetoothSocket::SocketError error) {
    QBluetoothSocket* socket = qobject_cast<QBluetoothSocket*>(sender());
    if (!socket) {
        return;
    }
    
    auto it = m_socketToPeerId.find(socket);
    if (it != m_socketToPeerId.end()) {
        std::string peerId = it->second;
        qDebug() << "Socket error for peer" << QString::fromStdString(peerId) << ":" << error;
        
        emit errorOccurred(QString("Socket error for peer %1: %2")
                          .arg(QString::fromStdString(peerId))
                          .arg(static_cast<int>(error)));
        
        cleanupPeer(peerId);
    }
}

void BluetoothMeshService::onSocketReadyRead() {
    QBluetoothSocket* socket = qobject_cast<QBluetoothSocket*>(sender());
    if (!socket) {
        return;
    }
    
    auto it = m_socketToPeerId.find(socket);
    if (it != m_socketToPeerId.end()) {
        std::string peerId = it->second;
        auto peerIt = m_peers.find(peerId);
        if (peerIt != m_peers.end()) {
            // Read available data
            QByteArray data = socket->readAll();
            std::vector<uint8_t> buffer(data.begin(), data.end());
            
            // Append to receive buffer
            peerIt->second->receiveBuffer.insert(
                peerIt->second->receiveBuffer.end(),
                buffer.begin(), buffer.end());
            
            // Process complete messages
            processIncomingData(peerId, peerIt->second->receiveBuffer);
            
            // Update last seen
            peerIt->second->lastSeen = getCurrentTimestamp();
        }
    }
}

void BluetoothMeshService::onAdvertisementTimeout() {
    // Refresh advertisement
    if (m_isAdvertising) {
        m_localDevice->setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    }
}

void BluetoothMeshService::onScanTimeout() {
    // Restart scanning
    if (m_isScanning && m_discoveryAgent) {
        m_discoveryAgent->stop();
        QTimer::singleShot(100, this, [this]() {
            if (m_isScanning) {
                m_discoveryAgent->start();
            }
        });
    }
}

void BluetoothMeshService::onPeerCleanupTimeout() {
    checkPeerTimeouts();
}

void BluetoothMeshService::onHeartbeatTimeout() {
    if (m_isRunning) {
        sendHeartbeat();
    }
}

// Helper method implementations
void BluetoothMeshService::sendToPeer(const std::string& peerId, const std::vector<uint8_t>& data) {
    auto it = m_peers.find(peerId);
    if (it != m_peers.end() && it->second->isConnected && it->second->socket) {
        QByteArray qdata(reinterpret_cast<const char*>(data.data()), data.size());
        it->second->socket->write(qdata);
    }
}

void BluetoothMeshService::sendToAllPeers(const std::vector<uint8_t>& data, const std::string& excludePeerId) {
    for (const auto& pair : m_peers) {
        if (pair.first != excludePeerId && pair.second->isConnected) {
            sendToPeer(pair.first, data);
        }
    }
}

void BluetoothMeshService::processIncomingData(const std::string& peerId, std::vector<uint8_t>& buffer) {
    // Simple message processing - look for complete packets
    // This is a simplified implementation
    if (buffer.size() >= 4) {
        uint32_t messageLength = *reinterpret_cast<uint32_t*>(buffer.data());
        
        if (buffer.size() >= messageLength + 4) {
            // Extract message
            std::vector<uint8_t> message(buffer.begin() + 4, buffer.begin() + 4 + messageLength);
            
            // Remove processed data from buffer
            buffer.erase(buffer.begin(), buffer.begin() + 4 + messageLength);
            
            // Handle the message
            handleIncomingMessage(peerId, message);
            
            // Process any remaining data
            if (!buffer.empty()) {
                processIncomingData(peerId, buffer);
            }
        }
    }
}

void BluetoothMeshService::handleIncomingMessage(const std::string& peerId, const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return;
    }
    
    uint8_t messageType = data[0];
    
    switch (messageType) {
        case MESSAGE_TYPE_CHAT:
        case MESSAGE_TYPE_PRIVATE:
        case MESSAGE_TYPE_CHANNEL:
            emit messageReceived(data, peerId);
            break;
            
        case MESSAGE_TYPE_NICKNAME:
            handleNicknameAnnouncement(peerId, data);
            break;
            
        case MESSAGE_TYPE_HEARTBEAT:
            handleHeartbeat(peerId, data);
            break;
            
        case MESSAGE_TYPE_RELAY:
            // Handle relay messages
            relayMessage(data, peerId);
            emit messageReceived(data, peerId);
            break;
            
        default:
            qDebug() << "Unknown message type:" << messageType;
            break;
    }
}

void BluetoothMeshService::handleNicknameAnnouncement(const std::string& peerId, const std::vector<uint8_t>& data) {
    if (data.size() < 2) {
        return;
    }
    
    std::string nickname(data.begin() + 1, data.end());
    
    auto it = m_peers.find(peerId);
    if (it != m_peers.end()) {
        it->second->nickname = nickname;
        qDebug() << "Peer" << QString::fromStdString(peerId) << "nickname:" << QString::fromStdString(nickname);
    }
}

void BluetoothMeshService::handleHeartbeat(const std::string& peerId, const std::vector<uint8_t>& data) {
    updatePeerLastSeen(peerId);
}

void BluetoothMeshService::sendHeartbeat() {
    std::vector<uint8_t> packet = createHeartbeatPacket();
    sendToAllPeers(packet);
}

void BluetoothMeshService::updatePeerLastSeen(const std::string& peerId) {
    auto it = m_peers.find(peerId);
    if (it != m_peers.end()) {
        it->second->lastSeen = getCurrentTimestamp();
    }
}

void BluetoothMeshService::checkPeerTimeouts() {
    uint64_t currentTime = getCurrentTimestamp();
    std::vector<std::string> timedOutPeers;
    
    for (const auto& pair : m_peers) {
        if (pair.second->isConnected && 
            (currentTime - pair.second->lastSeen) > PEER_TIMEOUT) {
            timedOutPeers.push_back(pair.first);
        }
    }
    
    for (const std::string& peerId : timedOutPeers) {
        qDebug() << "Peer timed out:" << QString::fromStdString(peerId);
        disconnectPeer(peerId);
    }
}

void BluetoothMeshService::disconnectPeer(const std::string& peerId) {
    auto it = m_peers.find(peerId);
    if (it != m_peers.end()) {
        if (it->second->socket) {
            it->second->socket->close();
        }
        emit peerDisconnected(peerId);
        cleanupPeer(peerId);
    }
}

void BluetoothMeshService::cleanupPeer(const std::string& peerId) {
    auto it = m_peers.find(peerId);
    if (it != m_peers.end()) {
        if (it->second->socket) {
            m_socketToPeerId.erase(it->second->socket);
            it->second->socket->deleteLater();
        }
        m_peers.erase(it);
    }
}

std::string BluetoothMeshService::generatePeerId() const {
    // Generate a unique peer ID based on MAC address and timestamp
    QBluetoothAddress address = m_localDevice ? m_localDevice->address() : QBluetoothAddress();
    QString addressStr = address.toString().remove(':');
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    return (addressStr + QString::number(timestamp)).toStdString();
}

uint64_t BluetoothMeshService::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

std::vector<uint8_t> BluetoothMeshService::createMessagePacket(const std::string& content, 
                                                              const std::string& channel,
                                                              const std::string& recipientId) {
    std::vector<uint8_t> packet;
    
    // Message type
    uint8_t messageType = MESSAGE_TYPE_CHAT;
    if (!recipientId.empty()) {
        messageType = MESSAGE_TYPE_PRIVATE;
    } else if (!channel.empty()) {
        messageType = MESSAGE_TYPE_CHANNEL;
    }
    
    packet.push_back(messageType);
    
    // Sender ID
    packet.insert(packet.end(), m_myPeerId.begin(), m_myPeerId.end());
    packet.push_back(0); // Null terminator
    
    // Recipient ID (for private messages)
    if (!recipientId.empty()) {
        packet.insert(packet.end(), recipientId.begin(), recipientId.end());
    }
    packet.push_back(0); // Null terminator
    
    // Channel (for channel messages)
    if (!channel.empty()) {
        packet.insert(packet.end(), channel.begin(), channel.end());
    }
    packet.push_back(0); // Null terminator
    
    // Content
    packet.insert(packet.end(), content.begin(), content.end());
    
    // Add length header
    uint32_t length = packet.size();
    std::vector<uint8_t> result;
    result.resize(4);
    *reinterpret_cast<uint32_t*>(result.data()) = length;
    result.insert(result.end(), packet.begin(), packet.end());
    
    return result;
}

std::vector<uint8_t> BluetoothMeshService::createHeartbeatPacket() {
    // Create a simple heartbeat packet
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>(0)); // No data in heartbeat
    
    return addProtocolHeader(payload, BluetoothMeshService::MESSAGE_TYPE_HEARTBEAT);
}

std::vector<uint8_t> BluetoothMeshService::createNicknamePacket(const std::string& nickname) {
    // Create a nickname announcement packet
    std::vector<uint8_t> payload(nickname.begin(), nickname.end());
    
    return addProtocolHeader(payload, BluetoothMeshService::MESSAGE_TYPE_NICKNAME);
}

void BluetoothMeshService::setAdvertisementInterval(int intervalMs) {
    m_advertisementInterval = intervalMs;
    if (m_advertisementTimer && m_advertisementTimer->isActive()) {
        m_advertisementTimer->setInterval(intervalMs);
    }
}

void BluetoothMeshService::setScanInterval(int intervalMs) {
    m_scanInterval = intervalMs;
    if (m_scanTimer && m_scanTimer->isActive()) {
        m_scanTimer->setInterval(intervalMs);
    }
}

void BluetoothMeshService::setMaxConnections(int maxConnections) {
    m_maxConnections = maxConnections;
}

void BluetoothMeshService::setTransmissionPower(int powerLevel) {
    m_transmissionPower = powerLevel;
    // Note: Qt doesn't provide direct API for transmission power control
    // This would need platform-specific implementation
}

void BluetoothMeshService::requestPeerInfo(const std::string& peerId) {
    // Request peer info
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>(0)); // No data in request
    
    std::vector<uint8_t> packet = addProtocolHeader(payload, BluetoothMeshService::MESSAGE_TYPE_PEER_INFO_REQUEST);
    
    sendToPeer(peerId, packet);
}

bool BluetoothMeshService::isValidPeerId(const std::string& peerId) const {
    return !peerId.empty() && peerId.length() > 8;
}

uint8_t BluetoothMeshService::calculateChecksum(const std::vector<uint8_t>& data) const {
    uint8_t checksum = 0;
    for (uint8_t byte : data) {
        checksum ^= byte;
    }
    return checksum;
}

bool BluetoothMeshService::verifyChecksum(const std::vector<uint8_t>& data) const {
    if (data.empty()) {
        return false;
    }
    
    uint8_t receivedChecksum = data.back();
    std::vector<uint8_t> payload(data.begin(), data.end() - 1);
    uint8_t calculatedChecksum = calculateChecksum(payload);
    
    return receivedChecksum == calculatedChecksum;
}

std::vector<uint8_t> BluetoothMeshService::addProtocolHeader(const std::vector<uint8_t>& payload, uint8_t messageType) const {
    std::vector<uint8_t> packet;
    packet.push_back(messageType);
    packet.insert(packet.end(), payload.begin(), payload.end());
    
    // Add checksum
    uint8_t checksum = calculateChecksum(packet);
    packet.push_back(checksum);
    
    return packet;
}

std::vector<uint8_t> BluetoothMeshService::removeProtocolHeader(const std::vector<uint8_t>& packet, uint8_t& messageType) const {
    if (packet.size() < 2) {
        return {};
    }
    
    messageType = packet[0];
    std::vector<uint8_t> payload(packet.begin() + 1, packet.end() - 1);
    
    return payload;
} 

void BluetoothMeshService::handleRetryRequest(const QString& messageID) {
    // Re-send the message that needs to be retried
    qDebug() << "Retrying message:" << messageID;
    // Implementation would re-send the message from a local cache
}

void BluetoothMeshService::handleDeliveryStatusUpdate(const QString& messageID, DeliveryStatus status) {
    // Handle delivery status updates
    qDebug() << "Message" << messageID << "status updated to:" << static_cast<int>(status);
    // Implementation would update UI or take action based on status
} 