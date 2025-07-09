#ifndef BITCHATAPPLICATION_H
#define BITCHATAPPLICATION_H

#include <QCoreApplication>
#include <memory>
#include <string>

// Forward declarations
class BluetoothMeshService;
class MessageHandler;
class NotificationService;
class DeliveryTracker;
class BatteryOptimizer;
class ConfigManager;
class MainWindow;

struct BitchatMessage;

struct PeerInfo {
    std::string id;
    std::string nickname;
    bool isConnected;
    uint64_t lastSeen;
    
    PeerInfo() : isConnected(false), lastSeen(0) {}
};

struct ChannelInfo {
    std::string name;
    int memberCount;
    bool isJoined;
    
    ChannelInfo() : memberCount(0), isJoined(false) {}
};

class BitchatApplication : public QCoreApplication
{
    Q_OBJECT

public:
    explicit BitchatApplication(int argc, char* argv[]);
    ~BitchatApplication();

    bool initialize();
    void run();
    void shutdown();
    
    // Configuration methods
    void setVerbose(bool verbose);
    void setConfigPath(const std::string& path);
    
    // Public methods for MainWindow
    ConfigManager* getConfigManager() const;
    void sendMessage(const std::string& message);
    int getPeerCount() const;
    QStringList getPeerList() const;
    QStringList getChannelList() const;

signals:
    void messageReceived(const BitchatMessage& message);
    void messageSent(const BitchatMessage& message);
    void peerConnected(const PeerInfo& peer);
    void peerDisconnected(const PeerInfo& peer);
    void channelJoined(const ChannelInfo& channel);
    void channelLeft(const std::string& channel);
    void connectionStatusChanged(bool connected);

private slots:
    void onMessageReceived(const std::string& message, const std::string& peerId);
    void onPeerConnected(const std::string& peerId);
    void onPeerDisconnected(const std::string& peerId);
    void onMessageDelivered(const QString& messageId, const QString& recipientId);
    void onMessageFailed(const QString& messageId, const QString& recipientId, const QString& error);
    void onBatteryLevelChanged(int level);
    void onPowerModeChanged(int mode);

private:
    void parseArguments();
    void initializeServices();
    void connectServices();
    void startInputProcessing();
    std::string formatMessage(const BitchatMessage& message);
    void showHelp();
    void showVersion();
    uint64_t getCurrentTimestamp() const;

    // Services
    BluetoothMeshService* m_meshService;
    ConfigManager* m_configManager;
    MessageHandler* m_messageHandler;
    DeliveryTracker* m_deliveryTracker;
    NotificationService* m_notificationService;
    BatteryOptimizer* m_batteryOptimizer;
    MainWindow* m_mainWindow;

    // Configuration
    std::string m_configPath;
    std::string m_nickname;
    bool m_isRunning;
    bool m_isDaemon;
    bool m_verbose;
};

#endif // BITCHATAPPLICATION_H 