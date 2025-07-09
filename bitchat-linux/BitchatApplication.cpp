#include "BitchatApplication.h"
#include "Services/BluetoothMeshService.h"
#include "ViewModels/ConfigManager.h"
#include "Services/NotificationService.h"
#include "Services/DeliveryTracker.h"
#include "ViewModels/MessageHandler.h"
#include "Services/MessageRetryService.h"
#include "Services/MessageRetentionService.h"
#include "Services/KeychainManager.h"
#include "Utils/OptimizedBloomFilter.h"
#include "Utils/BatteryOptimizer.h"
#include "Views/ui/MainWindow.h"

#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <random>
#include <signal.h>

BitchatApplication::BitchatApplication(int argc, char* argv[])
    : QCoreApplication(argc, argv)
    , m_meshService(nullptr)
    , m_configManager(nullptr)
    , m_messageHandler(nullptr)
    , m_deliveryTracker(nullptr)
    , m_notificationService(nullptr)
    , m_batteryOptimizer(nullptr)
    , m_mainWindow(nullptr)
    , m_isRunning(false)
    , m_isDaemon(false)
    , m_verbose(false)
{
    // Parse command line arguments
    parseArguments();
    
    // Initialize services
    initializeServices();
}

BitchatApplication::~BitchatApplication() {
    shutdown();
}

bool BitchatApplication::initialize() {
    if (m_verbose) {
        qDebug() << "Initializing BitChat application...";
    }
    
    // Load configuration
    if (!m_configManager->load()) {
        std::cerr << "Warning: Failed to load configuration, using defaults" << std::endl;
    }
    
    // Initialize notification service
    m_notificationService = new NotificationService(this);
    if (!m_notificationService->initialize()) {
        qWarning() << "Failed to initialize notification service";
    }
    
    // Initialize battery optimizer
    if (!m_batteryOptimizer->initialize()) {
        std::cerr << "Warning: Failed to initialize battery optimizer" << std::endl;
    }
    
    // Initialize delivery tracker
    m_deliveryTracker = DeliveryTracker::shared();
    if (!m_deliveryTracker) {
        qCritical() << "Failed to initialize delivery tracker";
        return false;
    }
    
    // Initialize mesh service
    if (!m_meshService->initialize()) {
        std::cerr << "Error: Failed to initialize Bluetooth mesh service" << std::endl;
        return false;
    }
    
    // Initialize GUI if not in daemon mode
    if (!m_isDaemon) {
        m_mainWindow = new MainWindow(this);
        // MainWindow doesn't have an initialize method, it's ready after construction
    }
    
    // Setup connections between services
    connectServices();
    
    if (m_verbose) {
        qDebug() << "BitChat application initialized successfully";
    }
    
    return true;
}

void BitchatApplication::run() {
    if (m_isRunning) {
        return;
    }
    
    m_isRunning = true;
    
    if (m_verbose) {
        qDebug() << "Starting BitChat application...";
    }
    
    // Start battery optimizer first
    // Battery optimizer is started automatically after initialization
    
    // Start mesh service
    m_meshService->start();
    
    // Delivery tracker is started automatically after initialization
    
    // Set nickname from config
    std::string nickname = m_configManager->getNickname();
    if (!nickname.empty()) {
        m_messageHandler->setNickname(nickname);
        m_meshService->announceNickname(nickname);
    }
    
    // Auto-join configured channels
    std::vector<std::string> channels = m_configManager->getJoinedChannels();
    for (const std::string& channel : channels) {
        m_messageHandler->joinChannel(channel);
    }
    
    // Show main window if not in daemon mode
    if (!m_isDaemon && m_mainWindow) {
        m_mainWindow->show();
    }
    
    std::cout << "BitChat is running. Type /help for commands." << std::endl;
    
    if (!m_isDaemon && !m_mainWindow) {
        // Start input processing in console mode
        startInputProcessing();
    }
    
    // Start Qt event loop
    exec();
}

void BitchatApplication::shutdown() {
    if (!m_isRunning) {
        return;
    }
    
    if (m_verbose) {
        qDebug() << "Shutting down BitChat application...";
    }
    
    m_isRunning = false;
    
    // Stop services in reverse order
    // Delivery tracker is stopped automatically during shutdown
    
    if (m_meshService) {
        m_meshService->stop();
    }
    
    // Battery optimizer is stopped automatically during shutdown
    
    if (m_notificationService) {
        m_notificationService->shutdown();
    }
    
    // Close main window
    if (m_mainWindow) {
        m_mainWindow->close();
    }
    
    // Save configuration
    if (m_configManager) {
        m_configManager->save();
    }
    
    // Quit Qt event loop
    quit();
}

void BitchatApplication::setVerbose(bool verbose) {
    m_verbose = verbose;
}

void BitchatApplication::setConfigPath(const std::string& path) {
    m_configPath = path;
}

// Public methods for MainWindow
ConfigManager* BitchatApplication::getConfigManager() const {
    return m_configManager;
}

void BitchatApplication::sendMessage(const std::string& message) {
    if (m_messageHandler) {
        m_messageHandler->processMessage(message);
    }
}

int BitchatApplication::getPeerCount() const {
    if (m_meshService) {
        return m_meshService->getConnectedPeerCount();
    }
    return 0;
}

QStringList BitchatApplication::getPeerList() const {
    QStringList result;
    if (m_meshService) {
        std::vector<ConnectedPeer> peers = m_meshService->getConnectedPeers();
        for (const auto& peer : peers) {
            QString peerName = QString::fromStdString(peer.nickname.empty() ? peer.id : peer.nickname);
            result.append(peerName);
        }
    }
    return result;
}

QStringList BitchatApplication::getChannelList() const {
    QStringList result;
    if (m_messageHandler) {
        std::set<std::string> channels = m_messageHandler->getJoinedChannels();
        for (const auto& channel : channels) {
            result.append(QString::fromStdString(channel));
        }
    }
    return result;
}

void BitchatApplication::parseArguments() {
    QStringList args = arguments();
    
    for (int i = 1; i < args.size(); ++i) {
        const QString& arg = args[i];
        
        if (arg == "--daemon" || arg == "-d") {
            m_isDaemon = true;
        } else if (arg == "--verbose" || arg == "-v") {
            m_verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            showHelp();
            exit(0);
        } else if (arg == "--version") {
            showVersion();
            exit(0);
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < args.size()) {
                m_configPath = args[++i].toStdString();
            } else {
                std::cerr << "Error: --config requires a path argument" << std::endl;
                exit(1);
            }
        } else if (arg == "--nickname" || arg == "-n") {
            if (i + 1 < args.size()) {
                m_nickname = args[++i].toStdString();
            } else {
                std::cerr << "Error: --nickname requires a name argument" << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Unknown argument: " << arg.toStdString() << std::endl;
            showHelp();
            exit(1);
        }
    }
}

void BitchatApplication::initializeServices() {
    // Initialize configuration manager
    m_configManager = new ConfigManager();
    if (!m_configPath.empty()) {
        m_configManager->setConfigPath(m_configPath);
    }
    
    // Initialize notification service
    m_notificationService = new NotificationService(this);
    if (!m_notificationService->initialize()) {
        qWarning() << "Failed to initialize notification service";
    }
    
    // Initialize battery optimizer
    m_batteryOptimizer = new BatteryOptimizer(m_configManager, this);
    
    // Initialize delivery tracker
    m_deliveryTracker = DeliveryTracker::shared();
    if (!m_deliveryTracker) {
        qCritical() << "Failed to initialize delivery tracker";
        return;
    }
    
    // Initialize mesh service
    m_meshService = new BluetoothMeshService(this);
    
    // Initialize message handler
    m_messageHandler = new MessageHandler(this);
    
    // Set nickname if provided via command line
    if (!m_nickname.empty()) {
        m_configManager->setNickname(m_nickname);
        m_messageHandler->setNickname(m_nickname);
    }
}

void BitchatApplication::connectServices() {
    // Connect mesh service to message handler
    connect(m_meshService, &BluetoothMeshService::messageReceived,
            m_messageHandler, &MessageHandler::processIncomingMessage);
    
    // Connect message handler to mesh service
    connect(m_messageHandler, &MessageHandler::messageToBroadcast,
            m_meshService, &BluetoothMeshService::sendBroadcast);
    
    connect(m_messageHandler, &MessageHandler::messageToChannel,
            [this](const std::vector<uint8_t>& data, const std::string& channel) {
                // Convert binary data back to string for the mesh service
                std::string content(data.begin(), data.end());
                m_meshService->sendMessage(content, channel);
            });
    
    connect(m_messageHandler, &MessageHandler::privateMessageToPeer,
            [this](const std::vector<uint8_t>& data, const std::string& peerId) {
                // Convert binary data back to string for the mesh service
                std::string content(data.begin(), data.end());
                m_meshService->sendPrivateMessage(content, peerId);
            });
    
    // Connect delivery tracker to mesh service
    /*
    connect(m_deliveryTracker, &DeliveryTracker::messageReadyForDelivery,
            [this](const QString& messageId, const std::vector<uint8_t>& data) {
                if (m_meshService) {
                    m_meshService->sendBroadcast(data);
                m_deliveryTracker->markMessageDelivered(messageId);
                }
            });
    */
    
    // Connect mesh service to delivery tracker
    /*
    connect(m_meshService, &BluetoothMeshService::peerConnected,
            [this](const std::string& peerId, const std::string&) {
                m_deliveryTracker->onPeerConnected(QString::fromStdString(peerId));
            });
    
    connect(m_meshService, &BluetoothMeshService::peerDisconnected,
            [this](const std::string& peerId) {
                m_deliveryTracker->onPeerDisconnected(QString::fromStdString(peerId));
            });
    */
    
    // Connect battery optimizer to mesh service
    connect(m_batteryOptimizer, &BatteryOptimizer::powerModeChanged,
            [this](PowerMode mode) {
                // Adjust mesh service parameters based on power mode
                switch (mode) {
                    case PowerMode::HIGH_PERFORMANCE:
                        m_meshService->setAdvertisementInterval(1000);
                        m_meshService->setScanInterval(2000);
                        m_meshService->setMaxConnections(10);
                        break;
                    case PowerMode::BALANCED:
                        m_meshService->setAdvertisementInterval(2000);
                        m_meshService->setScanInterval(4000);
                        m_meshService->setMaxConnections(8);
                        break;
                    case PowerMode::POWER_SAVER:
                        m_meshService->setAdvertisementInterval(5000);
                        m_meshService->setScanInterval(10000);
                        m_meshService->setMaxConnections(5);
                        break;
                    case PowerMode::ULTRA_LOW_POWER:
                        m_meshService->setAdvertisementInterval(10000);
                        m_meshService->setScanInterval(20000);
                        m_meshService->setMaxConnections(3);
                        break;
                }
            });
    
    // Connect notification service to message handler
    connect(m_messageHandler, &MessageHandler::messageReceived,
            [this](const BitchatMessage& message) {
                if (m_notificationService) {
                    m_notificationService->showMessageNotification(
                        QString::fromStdString(message.sender),
                        QString::fromStdString(message.content),
                        QString::fromStdString(message.channel)
                    );
                }
            });
    
    connect(m_messageHandler, &MessageHandler::privateMessageReceived,
            [this](const BitchatMessage& message) {
                if (m_notificationService) {
                    m_notificationService->showMessageNotification(
                        QString::fromStdString(message.sender),
                        QString::fromStdString(message.content),
                        "Private"
                    );
                }
            });
    
    // Connect mesh service to notifications
    connect(m_meshService, &BluetoothMeshService::peerConnected,
            [this](const std::string& peerId, const std::string& nickname) {
                if (m_notificationService) {
                    m_notificationService->showSystemNotification(
                        QString("Peer %1 connected").arg(QString::fromStdString(nickname.empty() ? peerId : nickname))
                    );
                }
            });
    
    connect(m_meshService, &BluetoothMeshService::peerDisconnected,
            [this](const std::string& peerId) {
                if (m_notificationService) {
                    m_notificationService->showSystemNotification(
                        QString("Peer %1 disconnected").arg(QString::fromStdString(peerId))
                    );
                }
            });
    
    // Connect main window if available
    if (m_mainWindow) {
        // MainWindow connections will be handled internally by the MainWindow class
        // The MainWindow will connect to the services directly as needed
    }
    
    // Forward signals to MainWindow
    connect(m_messageHandler, &MessageHandler::messageReceived,
            this, &BitchatApplication::messageReceived);
    connect(m_messageHandler, &MessageHandler::messageProcessed,
            this, &BitchatApplication::messageSent);
    
    connect(m_meshService, &BluetoothMeshService::peerConnected,
            [this](const std::string& peerId, const std::string& nickname) {
                PeerInfo peer;
                peer.id = peerId;
                peer.nickname = nickname;
                peer.isConnected = true;
                peer.lastSeen = getCurrentTimestamp();
                emit peerConnected(peer);
            });
    
    connect(m_meshService, &BluetoothMeshService::peerDisconnected,
            [this](const std::string& peerId) {
                PeerInfo peer;
                peer.id = peerId;
                peer.isConnected = false;
                peer.lastSeen = getCurrentTimestamp();
                emit peerDisconnected(peer);
            });
    
    connect(m_messageHandler, &MessageHandler::channelJoined,
            [this](const std::string& channel) {
                ChannelInfo channelInfo;
                channelInfo.name = channel;
                channelInfo.isJoined = true;
                channelInfo.memberCount = 1; // At least us
                emit channelJoined(channelInfo);
            });
    
    connect(m_messageHandler, &MessageHandler::channelLeft,
            this, &BitchatApplication::channelLeft);
    
    // Connection status changes
    connect(m_meshService, &BluetoothMeshService::peerConnected,
            [this](const std::string&, const std::string&) {
                emit connectionStatusChanged(true);
            });
    
    connect(m_meshService, &BluetoothMeshService::peerDisconnected,
            [this](const std::string&) {
                bool hasConnections = m_meshService->getConnectedPeerCount() > 0;
                emit connectionStatusChanged(hasConnections);
            });
    
    // Connect message handler commands to application
    connect(m_messageHandler, &MessageHandler::quitRequested,
            this, &BitchatApplication::shutdown);
    connect(m_messageHandler, &MessageHandler::nicknameChanged,
            [this](const std::string& nickname) {
                m_configManager->setNickname(nickname);
                m_meshService->announceNickname(nickname);
            });
    connect(m_messageHandler, &MessageHandler::channelJoined,
            [this](const std::string& channel) {
                m_configManager->addJoinedChannel(channel);
                if (m_notificationService) {
                    m_notificationService->showSystemNotification(
                        QString("Joined channel #%1").arg(QString::fromStdString(channel))
                    );
                }
            });
    connect(m_messageHandler, &MessageHandler::channelLeft,
            [this](const std::string& channel) {
                m_configManager->removeJoinedChannel(channel);
                if (m_notificationService) {
                    m_notificationService->showSystemNotification(
                        QString("Left channel #%1").arg(QString::fromStdString(channel))
                    );
                }
            });

    // Connect mesh service events to console output (if not daemon mode)
    if (!m_isDaemon) {
        connect(m_meshService, &BluetoothMeshService::peerConnected,
                [this](const std::string& peerId, const std::string& nickname) {
                    std::cout << "*** " << peerId << " connected";
                    if (!nickname.empty()) {
                        std::cout << " (" << nickname << ")";
                    }
                    std::cout << std::endl;
                });
        
        connect(m_meshService, &BluetoothMeshService::peerDisconnected,
                [this](const std::string& peerId) {
                    std::cout << "*** " << peerId << " disconnected" << std::endl;
                });
    }
    
    connect(m_meshService, &BluetoothMeshService::errorOccurred,
            [this](const QString& error) {
                if (m_verbose) {
                    std::cerr << "Mesh service error: " << error.toStdString() << std::endl;
                }
            });
    
    // Connect message handler events to console output (if not daemon mode)
    if (!m_isDaemon) {
        connect(m_messageHandler, &MessageHandler::messageReceived,
                [this](const BitchatMessage& message) {
                    std::cout << "[" << message.sender << "] " << message.content << std::endl;
                });
        
        connect(m_messageHandler, &MessageHandler::privateMessageReceived,
                [this](const BitchatMessage& message) {
                    std::cout << "[PRIVATE from " << message.sender << "] " << message.content << std::endl;
                });
        
        connect(m_messageHandler, &MessageHandler::channelMessageReceived,
                [this](const BitchatMessage& message) {
                    std::cout << "[" << message.channel << "] " << message.sender << ": " << message.content << std::endl;
                });
        
        connect(m_messageHandler, &MessageHandler::systemMessageReceived,
                [this](const BitchatMessage& message) {
                    std::cout << "[SYSTEM] " << message.content << std::endl;
                });
        
        connect(m_messageHandler, &MessageHandler::commandResult,
                [this](const std::string& result) {
                    std::cout << result << std::endl;
                });
    }
    
    // Handle command requests
    connect(m_messageHandler, &MessageHandler::requestPeerList,
            [this]() {
                std::vector<ConnectedPeer> peers = m_meshService->getConnectedPeers();
                std::string result = "Connected peers:\n";
                for (const auto& peer : peers) {
                    result += peer.id;
                    if (!peer.nickname.empty()) {
                        result += " (" + peer.nickname + ")";
                    }
                    result += "\n";
                }
                if (peers.empty()) {
                    result = "No peers connected.";
                }
                if (!m_isDaemon) {
                    std::cout << result << std::endl;
                }
            });
    
    connect(m_messageHandler, &MessageHandler::requestConnectionStatus,
            [this]() {
                bool connected = m_meshService->isConnected();
                int peerCount = m_meshService->getConnectedPeerCount();
                std::string result = "Status: " + std::string(connected ? "Connected" : "Disconnected");
                result += "\nPeers: " + std::to_string(peerCount);
                result += "\nNickname: " + m_messageHandler->getNickname();
                result += "\nChannel: " + m_messageHandler->getCurrentChannel();
                if (!m_isDaemon) {
                    std::cout << result << std::endl;
                }
            });
    
    connect(m_messageHandler, &MessageHandler::blockPeerRequested,
            [this](const std::string& peerId) {
                m_configManager->addBlockedPeer(peerId);
                if (!m_isDaemon) {
                    std::cout << "Blocked peer: " << peerId << std::endl;
                }
            });
    
    connect(m_messageHandler, &MessageHandler::unblockPeerRequested,
            [this](const std::string& peerId) {
                m_configManager->removeBlockedPeer(peerId);
                if (!m_isDaemon) {
                    std::cout << "Unblocked peer: " << peerId << std::endl;
                }
            });
    
    connect(m_messageHandler, &MessageHandler::requestRelayStatus,
            [this]() {
                bool enabled = m_configManager->isRelayEnabled();
                if (!m_isDaemon) {
                    std::cout << "Message relay: " << (enabled ? "enabled" : "disabled") << std::endl;
                }
            });
    
    connect(m_messageHandler, &MessageHandler::setRelayEnabled,
            [this](bool enabled) {
                m_configManager->setRelayEnabled(enabled);
                if (!m_isDaemon) {
                    std::cout << "Message relay " << (enabled ? "enabled" : "disabled") << std::endl;
                }
            });
    
    connect(m_messageHandler, &MessageHandler::requestEncryptionStatus,
            [this]() {
                bool enabled = m_configManager->isEncryptionEnabled();
                if (!m_isDaemon) {
                    std::cout << "Encryption: " << (enabled ? "enabled" : "disabled") << std::endl;
                }
            });
    
    connect(m_messageHandler, &MessageHandler::setEncryptionEnabled,
            [this](bool enabled) {
                m_configManager->setEncryptionEnabled(enabled);
                if (!m_isDaemon) {
                    std::cout << "Encryption " << (enabled ? "enabled" : "disabled") << std::endl;
                }
            });
    
    // Connect delivery tracker events
    // Connect delivery tracker to UI
    /*
    connect(m_deliveryTracker, &DeliveryTracker::messageDelivered,
            this, &BitchatApplication::onMessageDelivered);
    
    connect(m_deliveryTracker, &DeliveryTracker::messageFailed,
            this, &BitchatApplication::onMessageFailed);
    */
    
    // Connect battery optimizer events
    connect(m_batteryOptimizer, &BatteryOptimizer::batteryLevelChanged,
            [this](int level) {
                if (m_verbose) {
                    qDebug() << "Battery level:" << level << "%";
                }
            });
    
    connect(m_batteryOptimizer, &BatteryOptimizer::batteryLow,
            [this]() {
                if (m_notificationService) {
                    m_notificationService->showSystemNotification("Battery low - switching to power saving mode");
                }
            });
    
    connect(m_batteryOptimizer, &BatteryOptimizer::batteryCritical,
            [this]() {
                if (m_notificationService) {
                    m_notificationService->showSystemNotification("Critical battery level - switching to ultra low power mode");
                }
            });
}

void BitchatApplication::startInputProcessing() {
    // Start a timer to periodically check for stdin input
    QTimer* inputTimer = new QTimer(this);
    connect(inputTimer, &QTimer::timeout, [this]() {
        // Check if there's input available
        if (std::cin.rdbuf()->in_avail() > 0) {
            std::string input;
            std::getline(std::cin, input);
            
            if (!input.empty()) {
                m_messageHandler->processMessage(input);
            }
        }
    });
    inputTimer->start(100); // Check every 100ms
}

void BitchatApplication::showHelp() {
    std::cout << "BitChat - Decentralized Mesh Messaging over Bluetooth LE" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: bitchat-linux [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -d, --daemon          Run in daemon mode (no console input)" << std::endl;
    std::cout << "  -v, --verbose         Enable verbose logging" << std::endl;
    std::cout << "  -c, --config PATH     Use custom configuration file" << std::endl;
    std::cout << "  -n, --nickname NAME   Set nickname" << std::endl;
    std::cout << "  -h, --help            Show this help message" << std::endl;
    std::cout << "  --version             Show version information" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands (in interactive mode):" << std::endl;
    std::cout << "  /help                 Show available commands" << std::endl;
    std::cout << "  /join #channel        Join a channel" << std::endl;
    std::cout << "  /msg <peer> <text>    Send private message" << std::endl;
    std::cout << "  /nick <name>          Set nickname" << std::endl;
    std::cout << "  /list                 List connected peers" << std::endl;
    std::cout << "  /status               Show connection status" << std::endl;
    std::cout << "  /quit                 Quit the application" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: This application requires CAP_NET_RAW capability for Bluetooth LE access." << std::endl;
    std::cout << "Run with: sudo setcap cap_net_raw+ep ./bitchat-linux" << std::endl;
}

void BitchatApplication::showVersion() {
    std::cout << "BitChat Linux v1.0.0" << std::endl;
    std::cout << "Decentralized mesh messaging over Bluetooth LE" << std::endl;
    std::cout << "Built with Qt6, libsodium, and BlueZ" << std::endl;
}

uint64_t BitchatApplication::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Private slots implementation
void BitchatApplication::onMessageReceived(const std::string& message, const std::string& peerId) {
    if (m_verbose) {
        std::cout << "Received message from " << peerId << ": " << message << std::endl;
    }
}

void BitchatApplication::onPeerConnected(const std::string& peerId) {
    if (m_verbose) {
        std::cout << "Peer connected: " << peerId << std::endl;
    }
}

void BitchatApplication::onPeerDisconnected(const std::string& peerId) {
    if (m_verbose) {
        std::cout << "Peer disconnected: " << peerId << std::endl;
    }
}

void BitchatApplication::onMessageDelivered(const QString& messageId, const QString& recipientId) {
    if (m_verbose) {
        qDebug() << "Message delivered:" << messageId << "to" << recipientId;
    }
}

void BitchatApplication::onMessageFailed(const QString& messageId, const QString& recipientId, const QString& error) {
    if (m_verbose) {
        qDebug() << "Message failed:" << messageId << "to" << recipientId << "error:" << error;
    }
}

void BitchatApplication::onBatteryLevelChanged(int level) {
    if (m_verbose) {
        qDebug() << "Battery level changed:" << level << "%";
    }
}

void BitchatApplication::onPowerModeChanged(int mode) {
    if (m_verbose) {
        qDebug() << "Power mode changed:" << mode;
    }
} 