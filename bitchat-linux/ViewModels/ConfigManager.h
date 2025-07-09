#pragma once

#include <string>
#include <vector>
#include <map>

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // Configuration file management
    bool load();
    bool save();
    void setConfigPath(const std::string& path);
    std::string getConfigPath() const;
    
    // User settings
    std::string getNickname() const;
    void setNickname(const std::string& nickname);
    
    // Peer management
    std::vector<std::string> getBlockedPeers() const;
    void addBlockedPeer(const std::string& peerId);
    void removeBlockedPeer(const std::string& peerId);
    bool isBlockedPeer(const std::string& peerId) const;
    
    std::vector<std::string> getFavoritePeers() const;
    void addFavoritePeer(const std::string& peerId);
    void removeFavoritePeer(const std::string& peerId);
    bool isFavoritePeer(const std::string& peerId) const;
    
    // Channel management
    std::vector<std::string> getJoinedChannels() const;
    void addJoinedChannel(const std::string& channel);
    void removeJoinedChannel(const std::string& channel);
    bool isJoinedChannel(const std::string& channel) const;
    
    std::map<std::string, std::string> getChannelPasswords() const;
    void setChannelPassword(const std::string& channel, const std::string& password);
    std::string getChannelPassword(const std::string& channel) const;
    
    // Bluetooth settings
    int getAdvertisementInterval() const;
    void setAdvertisementInterval(int intervalMs);
    
    int getScanInterval() const;
    void setScanInterval(int intervalMs);
    
    int getMaxConnections() const;
    void setMaxConnections(int maxConnections);
    
    int getTransmissionPower() const;
    void setTransmissionPower(int powerLevel);
    
    // Security settings
    bool isEncryptionEnabled() const;
    void setEncryptionEnabled(bool enabled);
    
    std::string getKeyFilePath() const;
    void setKeyFilePath(const std::string& path);
    
    // Notification settings
    bool areNotificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);
    
    bool isNotificationSoundEnabled() const;
    void setNotificationSoundEnabled(bool enabled);
    
    // Battery optimization
    bool isBatteryOptimizationEnabled() const;
    void setBatteryOptimizationEnabled(bool enabled);
    
    int getLowBatteryThreshold() const;
    void setLowBatteryThreshold(int percentage);
    
    // Message settings
    int getMessageRetentionDays() const;
    void setMessageRetentionDays(int days);
    
    int getMaxMessageSize() const;
    void setMaxMessageSize(int sizeBytes);
    
    // Logging settings
    bool isLoggingEnabled() const;
    void setLoggingEnabled(bool enabled);
    
    std::string getLogFilePath() const;
    void setLogFilePath(const std::string& path);
    
    int getLogLevel() const;
    void setLogLevel(int level);
    
    // UI settings
    bool isDarkModeEnabled() const;
    void setDarkModeEnabled(bool enabled);
    
    std::string getFontFamily() const;
    void setFontFamily(const std::string& family);
    
    int getFontSize() const;
    void setFontSize(int size);
    
    // Network settings
    bool isAutoConnectEnabled() const;
    void setAutoConnectEnabled(bool enabled);
    
    bool isRelayEnabled() const;
    void setRelayEnabled(bool enabled);
    
    int getConnectionTimeout() const;
    void setConnectionTimeout(int timeoutMs);
    
    // Reset to defaults
    void resetToDefaults();
    
private:
    std::string m_configPath;
    std::map<std::string, std::string> m_settings;
    
    // Default values
    static const std::string DEFAULT_NICKNAME;
    static const int DEFAULT_ADVERTISEMENT_INTERVAL = 1000;
    static const int DEFAULT_SCAN_INTERVAL = 5000;
    static const int DEFAULT_MAX_CONNECTIONS = 8;
    static const int DEFAULT_TRANSMISSION_POWER = 0;
    static const bool DEFAULT_ENCRYPTION_ENABLED = true;
    static const bool DEFAULT_NOTIFICATIONS_ENABLED = true;
    static const bool DEFAULT_NOTIFICATION_SOUND_ENABLED = true;
    static const bool DEFAULT_BATTERY_OPTIMIZATION_ENABLED = true;
    static const int DEFAULT_LOW_BATTERY_THRESHOLD = 20;
    static const int DEFAULT_MESSAGE_RETENTION_DAYS = 30;
    static const int DEFAULT_MAX_MESSAGE_SIZE = 512;
    static const bool DEFAULT_LOGGING_ENABLED = false;
    static const int DEFAULT_LOG_LEVEL = 2; // INFO
    static const bool DEFAULT_DARK_MODE_ENABLED = true;
    static const std::string DEFAULT_FONT_FAMILY;
    static const int DEFAULT_FONT_SIZE = 12;
    static const bool DEFAULT_AUTO_CONNECT_ENABLED = true;
    static const bool DEFAULT_RELAY_ENABLED = true;
    static const int DEFAULT_CONNECTION_TIMEOUT = 30000;
    
    // Helper methods
    std::string getDefaultConfigPath() const;
    void setDefaults();
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);
    std::vector<std::string> getStringList(const std::string& key) const;
    void setStringList(const std::string& key, const std::vector<std::string>& values);
    std::map<std::string, std::string> getStringMap(const std::string& key) const;
    void setStringMap(const std::string& key, const std::map<std::string, std::string>& values);
}; 