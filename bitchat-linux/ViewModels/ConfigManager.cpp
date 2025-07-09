#include "ConfigManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

// Default values
const std::string ConfigManager::DEFAULT_NICKNAME = "anon";
const std::string ConfigManager::DEFAULT_FONT_FAMILY = "monospace";

ConfigManager::ConfigManager() {
    m_configPath = getDefaultConfigPath();
    setDefaults();
}

ConfigManager::~ConfigManager() {
    save();
}

bool ConfigManager::load() {
    QFile file(QString::fromStdString(m_configPath));
    if (!file.exists()) {
        qDebug() << "Config file doesn't exist, using defaults";
        return true; // Not an error, just use defaults
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open config file for reading:" << QString::fromStdString(m_configPath);
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse config file:" << error.errorString();
        return false;
    }
    
    if (!doc.isObject()) {
        qDebug() << "Config file is not a JSON object";
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Load settings
    for (auto it = root.begin(); it != root.end(); ++it) {
        QString key = it.key();
        QJsonValue value = it.value();
        
        if (value.isString()) {
            m_settings[key.toStdString()] = value.toString().toStdString();
        } else if (value.isBool()) {
            m_settings[key.toStdString()] = value.toBool() ? "true" : "false";
        } else if (value.isDouble()) {
            m_settings[key.toStdString()] = std::to_string(value.toInt());
        } else if (value.isArray()) {
            // Handle arrays (for lists like blocked peers, channels, etc.)
            QJsonArray array = value.toArray();
            std::string arrayStr;
            for (int i = 0; i < array.size(); ++i) {
                if (i > 0) arrayStr += ",";
                arrayStr += array[i].toString().toStdString();
            }
            m_settings[key.toStdString()] = arrayStr;
        } else if (value.isObject()) {
            // Handle objects (for maps like channel passwords)
            QJsonObject obj = value.toObject();
            std::string objStr;
            for (auto objIt = obj.begin(); objIt != obj.end(); ++objIt) {
                if (objIt != obj.begin()) objStr += ",";
                objStr += objIt.key().toStdString() + ":" + objIt.value().toString().toStdString();
            }
            m_settings[key.toStdString()] = objStr;
        }
    }
    
    qDebug() << "Loaded configuration from" << QString::fromStdString(m_configPath);
    return true;
}

bool ConfigManager::save() {
    // Create directory if it doesn't exist
    QDir dir(QString::fromStdString(m_configPath));
    if (!dir.exists()) {
        dir.mkpath(dir.absolutePath());
    }
    
    QJsonObject root;
    
    // Convert settings to JSON
    for (const auto& pair : m_settings) {
        QString key = QString::fromStdString(pair.first);
        QString value = QString::fromStdString(pair.second);
        
        // Handle different data types
        if (key.endsWith("_enabled") || key.endsWith("_mode") || key == "auto_connect" || 
            key == "relay_enabled" || key == "encryption_enabled" || key == "logging_enabled" ||
            key == "notifications_enabled" || key == "notification_sound_enabled" ||
            key == "battery_optimization_enabled" || key == "dark_mode_enabled") {
            // Boolean values
            root[key] = (value == "true");
        } else if (key.endsWith("_interval") || key.endsWith("_timeout") || key.endsWith("_threshold") ||
                   key.endsWith("_level") || key.endsWith("_power") || key.endsWith("_connections") ||
                   key.endsWith("_days") || key.endsWith("_size") || key == "font_size" ||
                   key == "log_level" || key == "max_connections" || key == "transmission_power") {
            // Integer values
            root[key] = value.toInt();
        } else if (key == "blocked_peers" || key == "favorite_peers" || key == "joined_channels") {
            // Array values
            QJsonArray array;
            if (!value.isEmpty()) {
                QStringList items = value.split(',');
                for (const QString& item : items) {
                    if (!item.trimmed().isEmpty()) {
                        array.append(item.trimmed());
                    }
                }
            }
            root[key] = array;
        } else if (key == "channel_passwords") {
            // Object values (maps)
            QJsonObject obj;
            if (!value.isEmpty()) {
                QStringList pairs = value.split(',');
                for (const QString& pair : pairs) {
                    QStringList keyValue = pair.split(':');
                    if (keyValue.size() == 2) {
                        obj[keyValue[0].trimmed()] = keyValue[1].trimmed();
                    }
                }
            }
            root[key] = obj;
        } else {
            // String values
            root[key] = value;
        }
    }
    
    QJsonDocument doc(root);
    
    QFile file(QString::fromStdString(m_configPath));
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open config file for writing:" << QString::fromStdString(m_configPath);
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    qDebug() << "Saved configuration to" << QString::fromStdString(m_configPath);
    return true;
}

void ConfigManager::setConfigPath(const std::string& path) {
    m_configPath = path;
}

std::string ConfigManager::getConfigPath() const {
    return m_configPath;
}

// User settings
std::string ConfigManager::getNickname() const {
    return getString("nickname", DEFAULT_NICKNAME);
}

void ConfigManager::setNickname(const std::string& nickname) {
    setString("nickname", nickname);
}

// Peer management
std::vector<std::string> ConfigManager::getBlockedPeers() const {
    return getStringList("blocked_peers");
}

void ConfigManager::addBlockedPeer(const std::string& peerId) {
    std::vector<std::string> blocked = getBlockedPeers();
    if (std::find(blocked.begin(), blocked.end(), peerId) == blocked.end()) {
        blocked.push_back(peerId);
        setStringList("blocked_peers", blocked);
    }
}

void ConfigManager::removeBlockedPeer(const std::string& peerId) {
    std::vector<std::string> blocked = getBlockedPeers();
    blocked.erase(std::remove(blocked.begin(), blocked.end(), peerId), blocked.end());
    setStringList("blocked_peers", blocked);
}

bool ConfigManager::isBlockedPeer(const std::string& peerId) const {
    std::vector<std::string> blocked = getBlockedPeers();
    return std::find(blocked.begin(), blocked.end(), peerId) != blocked.end();
}

std::vector<std::string> ConfigManager::getFavoritePeers() const {
    return getStringList("favorite_peers");
}

void ConfigManager::addFavoritePeer(const std::string& peerId) {
    std::vector<std::string> favorites = getFavoritePeers();
    if (std::find(favorites.begin(), favorites.end(), peerId) == favorites.end()) {
        favorites.push_back(peerId);
        setStringList("favorite_peers", favorites);
    }
}

void ConfigManager::removeFavoritePeer(const std::string& peerId) {
    std::vector<std::string> favorites = getFavoritePeers();
    favorites.erase(std::remove(favorites.begin(), favorites.end(), peerId), favorites.end());
    setStringList("favorite_peers", favorites);
}

bool ConfigManager::isFavoritePeer(const std::string& peerId) const {
    std::vector<std::string> favorites = getFavoritePeers();
    return std::find(favorites.begin(), favorites.end(), peerId) != favorites.end();
}

// Channel management
std::vector<std::string> ConfigManager::getJoinedChannels() const {
    return getStringList("joined_channels");
}

void ConfigManager::addJoinedChannel(const std::string& channel) {
    std::vector<std::string> channels = getJoinedChannels();
    if (std::find(channels.begin(), channels.end(), channel) == channels.end()) {
        channels.push_back(channel);
        setStringList("joined_channels", channels);
    }
}

void ConfigManager::removeJoinedChannel(const std::string& channel) {
    std::vector<std::string> channels = getJoinedChannels();
    channels.erase(std::remove(channels.begin(), channels.end(), channel), channels.end());
    setStringList("joined_channels", channels);
}

bool ConfigManager::isJoinedChannel(const std::string& channel) const {
    std::vector<std::string> channels = getJoinedChannels();
    return std::find(channels.begin(), channels.end(), channel) != channels.end();
}

std::map<std::string, std::string> ConfigManager::getChannelPasswords() const {
    return getStringMap("channel_passwords");
}

void ConfigManager::setChannelPassword(const std::string& channel, const std::string& password) {
    std::map<std::string, std::string> passwords = getChannelPasswords();
    passwords[channel] = password;
    setStringMap("channel_passwords", passwords);
}

std::string ConfigManager::getChannelPassword(const std::string& channel) const {
    std::map<std::string, std::string> passwords = getChannelPasswords();
    auto it = passwords.find(channel);
    return (it != passwords.end()) ? it->second : "";
}

// Bluetooth settings
int ConfigManager::getAdvertisementInterval() const {
    return getInt("advertisement_interval", DEFAULT_ADVERTISEMENT_INTERVAL);
}

void ConfigManager::setAdvertisementInterval(int intervalMs) {
    setInt("advertisement_interval", intervalMs);
}

int ConfigManager::getScanInterval() const {
    return getInt("scan_interval", DEFAULT_SCAN_INTERVAL);
}

void ConfigManager::setScanInterval(int intervalMs) {
    setInt("scan_interval", intervalMs);
}

int ConfigManager::getMaxConnections() const {
    return getInt("max_connections", DEFAULT_MAX_CONNECTIONS);
}

void ConfigManager::setMaxConnections(int maxConnections) {
    setInt("max_connections", maxConnections);
}

int ConfigManager::getTransmissionPower() const {
    return getInt("transmission_power", DEFAULT_TRANSMISSION_POWER);
}

void ConfigManager::setTransmissionPower(int powerLevel) {
    setInt("transmission_power", powerLevel);
}

// Security settings
bool ConfigManager::isEncryptionEnabled() const {
    return getBool("encryption_enabled", DEFAULT_ENCRYPTION_ENABLED);
}

void ConfigManager::setEncryptionEnabled(bool enabled) {
    setBool("encryption_enabled", enabled);
}

std::string ConfigManager::getKeyFilePath() const {
    return getString("key_file_path", getDefaultConfigPath() + "/keys.dat");
}

void ConfigManager::setKeyFilePath(const std::string& path) {
    setString("key_file_path", path);
}

// Notification settings
bool ConfigManager::areNotificationsEnabled() const {
    return getBool("notifications_enabled", DEFAULT_NOTIFICATIONS_ENABLED);
}

void ConfigManager::setNotificationsEnabled(bool enabled) {
    setBool("notifications_enabled", enabled);
}

bool ConfigManager::isNotificationSoundEnabled() const {
    return getBool("notification_sound_enabled", DEFAULT_NOTIFICATION_SOUND_ENABLED);
}

void ConfigManager::setNotificationSoundEnabled(bool enabled) {
    setBool("notification_sound_enabled", enabled);
}

// Battery optimization
bool ConfigManager::isBatteryOptimizationEnabled() const {
    return getBool("battery_optimization_enabled", DEFAULT_BATTERY_OPTIMIZATION_ENABLED);
}

void ConfigManager::setBatteryOptimizationEnabled(bool enabled) {
    setBool("battery_optimization_enabled", enabled);
}

int ConfigManager::getLowBatteryThreshold() const {
    return getInt("low_battery_threshold", DEFAULT_LOW_BATTERY_THRESHOLD);
}

void ConfigManager::setLowBatteryThreshold(int percentage) {
    setInt("low_battery_threshold", percentage);
}

// Message settings
int ConfigManager::getMessageRetentionDays() const {
    return getInt("message_retention_days", DEFAULT_MESSAGE_RETENTION_DAYS);
}

void ConfigManager::setMessageRetentionDays(int days) {
    setInt("message_retention_days", days);
}

int ConfigManager::getMaxMessageSize() const {
    return getInt("max_message_size", DEFAULT_MAX_MESSAGE_SIZE);
}

void ConfigManager::setMaxMessageSize(int sizeBytes) {
    setInt("max_message_size", sizeBytes);
}

// Logging settings
bool ConfigManager::isLoggingEnabled() const {
    return getBool("logging_enabled", DEFAULT_LOGGING_ENABLED);
}

void ConfigManager::setLoggingEnabled(bool enabled) {
    setBool("logging_enabled", enabled);
}

std::string ConfigManager::getLogFilePath() const {
    return getString("log_file_path", getDefaultConfigPath() + "/bitchat.log");
}

void ConfigManager::setLogFilePath(const std::string& path) {
    setString("log_file_path", path);
}

int ConfigManager::getLogLevel() const {
    return getInt("log_level", DEFAULT_LOG_LEVEL);
}

void ConfigManager::setLogLevel(int level) {
    setInt("log_level", level);
}

// UI settings
bool ConfigManager::isDarkModeEnabled() const {
    return getBool("dark_mode_enabled", DEFAULT_DARK_MODE_ENABLED);
}

void ConfigManager::setDarkModeEnabled(bool enabled) {
    setBool("dark_mode_enabled", enabled);
}

std::string ConfigManager::getFontFamily() const {
    return getString("font_family", DEFAULT_FONT_FAMILY);
}

void ConfigManager::setFontFamily(const std::string& family) {
    setString("font_family", family);
}

int ConfigManager::getFontSize() const {
    return getInt("font_size", DEFAULT_FONT_SIZE);
}

void ConfigManager::setFontSize(int size) {
    setInt("font_size", size);
}

// Network settings
bool ConfigManager::isAutoConnectEnabled() const {
    return getBool("auto_connect_enabled", DEFAULT_AUTO_CONNECT_ENABLED);
}

void ConfigManager::setAutoConnectEnabled(bool enabled) {
    setBool("auto_connect_enabled", enabled);
}

bool ConfigManager::isRelayEnabled() const {
    return getBool("relay_enabled", DEFAULT_RELAY_ENABLED);
}

void ConfigManager::setRelayEnabled(bool enabled) {
    setBool("relay_enabled", enabled);
}

int ConfigManager::getConnectionTimeout() const {
    return getInt("connection_timeout", DEFAULT_CONNECTION_TIMEOUT);
}

void ConfigManager::setConnectionTimeout(int timeoutMs) {
    setInt("connection_timeout", timeoutMs);
}

void ConfigManager::resetToDefaults() {
    m_settings.clear();
    setDefaults();
}

// Private helper methods
std::string ConfigManager::getDefaultConfigPath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return (configDir + "/bitchat/config.json").toStdString();
}

void ConfigManager::setDefaults() {
    // Set default values if not already set
    if (getString("nickname").empty()) {
        setString("nickname", DEFAULT_NICKNAME);
    }
    
    // Set other defaults
    setInt("advertisement_interval", DEFAULT_ADVERTISEMENT_INTERVAL);
    setInt("scan_interval", DEFAULT_SCAN_INTERVAL);
    setInt("max_connections", DEFAULT_MAX_CONNECTIONS);
    setInt("transmission_power", DEFAULT_TRANSMISSION_POWER);
    setBool("encryption_enabled", DEFAULT_ENCRYPTION_ENABLED);
    setBool("notifications_enabled", DEFAULT_NOTIFICATIONS_ENABLED);
    setBool("notification_sound_enabled", DEFAULT_NOTIFICATION_SOUND_ENABLED);
    setBool("battery_optimization_enabled", DEFAULT_BATTERY_OPTIMIZATION_ENABLED);
    setInt("low_battery_threshold", DEFAULT_LOW_BATTERY_THRESHOLD);
    setInt("message_retention_days", DEFAULT_MESSAGE_RETENTION_DAYS);
    setInt("max_message_size", DEFAULT_MAX_MESSAGE_SIZE);
    setBool("logging_enabled", DEFAULT_LOGGING_ENABLED);
    setInt("log_level", DEFAULT_LOG_LEVEL);
    setBool("dark_mode_enabled", DEFAULT_DARK_MODE_ENABLED);
    setString("font_family", DEFAULT_FONT_FAMILY);
    setInt("font_size", DEFAULT_FONT_SIZE);
    setBool("auto_connect_enabled", DEFAULT_AUTO_CONNECT_ENABLED);
    setBool("relay_enabled", DEFAULT_RELAY_ENABLED);
    setInt("connection_timeout", DEFAULT_CONNECTION_TIMEOUT);
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_settings.find(key);
    return (it != m_settings.end()) ? it->second : defaultValue;
}

int ConfigManager::getInt(const std::string& key, int defaultValue) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        return it->second == "true";
    }
    return defaultValue;
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    m_settings[key] = value;
}

void ConfigManager::setInt(const std::string& key, int value) {
    m_settings[key] = std::to_string(value);
}

void ConfigManager::setBool(const std::string& key, bool value) {
    m_settings[key] = value ? "true" : "false";
}

std::vector<std::string> ConfigManager::getStringList(const std::string& key) const {
    std::vector<std::string> result;
    std::string value = getString(key);
    
    if (!value.empty()) {
        size_t start = 0;
        size_t end = value.find(',');
        
        while (end != std::string::npos) {
            std::string item = value.substr(start, end - start);
            if (!item.empty()) {
                result.push_back(item);
            }
            start = end + 1;
            end = value.find(',', start);
        }
        
        // Add the last item
        std::string item = value.substr(start);
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

void ConfigManager::setStringList(const std::string& key, const std::vector<std::string>& values) {
    std::string result;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) result += ",";
        result += values[i];
    }
    setString(key, result);
}

std::map<std::string, std::string> ConfigManager::getStringMap(const std::string& key) const {
    std::map<std::string, std::string> result;
    std::string value = getString(key);
    
    if (!value.empty()) {
        size_t start = 0;
        size_t end = value.find(',');
        
        while (end != std::string::npos) {
            std::string pair = value.substr(start, end - start);
            size_t colonPos = pair.find(':');
            if (colonPos != std::string::npos) {
                std::string mapKey = pair.substr(0, colonPos);
                std::string mapValue = pair.substr(colonPos + 1);
                result[mapKey] = mapValue;
            }
            start = end + 1;
            end = value.find(',', start);
        }
        
        // Add the last pair
        std::string pair = value.substr(start);
        size_t colonPos = pair.find(':');
        if (colonPos != std::string::npos) {
            std::string mapKey = pair.substr(0, colonPos);
            std::string mapValue = pair.substr(colonPos + 1);
            result[mapKey] = mapValue;
        }
    }
    
    return result;
}

void ConfigManager::setStringMap(const std::string& key, const std::map<std::string, std::string>& values) {
    std::string result;
    bool first = true;
    for (const auto& pair : values) {
        if (!first) result += ",";
        result += pair.first + ":" + pair.second;
        first = false;
    }
    setString(key, result);
} 