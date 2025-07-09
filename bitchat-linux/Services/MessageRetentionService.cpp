#include "MessageRetentionService.h"
#include "KeychainManager.h"
#include "BinaryProtocol.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QFileInfo>
#include <QDirIterator>
#include <QCryptographicHash>
#include <sodium.h>

// Settings keys
const QString MessageRetentionService::FAVORITE_CHANNELS_KEY = "favoriteChannels";
const QString MessageRetentionService::RETENTION_DAYS_KEY = "retentionDays";

MessageRetentionService* MessageRetentionService::s_instance = nullptr;

MessageRetentionService::MessageRetentionService(QObject* parent)
    : QObject(parent)
    , m_retentionDays(DEFAULT_RETENTION_DAYS)
    , m_cleanupTimer(nullptr)
    , m_keychain(KeychainManager::shared())
{
    // Initialize libsodium
    if (sodium_init() < 0) {
        qWarning() << "Failed to initialize libsodium for MessageRetentionService";
    }
    
    // Set up storage directory
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_storageDirectory = appDataDir + "/messages";
    
    QDir dir;
    if (!dir.mkpath(m_storageDirectory)) {
        qWarning() << "Failed to create message storage directory:" << m_storageDirectory;
    }
    
    // Load settings
    loadSettings();
    
    // Set up cleanup timer (run every hour)
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setSingleShot(false);
    m_cleanupTimer->setInterval(60 * 60 * 1000); // 1 hour
    connect(m_cleanupTimer, &QTimer::timeout, this, &MessageRetentionService::cleanupOldMessages);
    m_cleanupTimer->start();
    
    // Run initial cleanup
    cleanupOldMessages();
}

MessageRetentionService::~MessageRetentionService()
{
    saveSettings();
    
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
    }
}

MessageRetentionService* MessageRetentionService::shared()
{
    if (!s_instance) {
        s_instance = new MessageRetentionService();
    }
    return s_instance;
}

QSet<QString> MessageRetentionService::getFavoriteChannels() const
{
    return m_favoriteChannels;
}

bool MessageRetentionService::toggleFavoriteChannel(const QString& channel)
{
    if (channel.isEmpty()) {
        return false;
    }
    
    bool wasFavorite = m_favoriteChannels.contains(channel);
    
    if (wasFavorite) {
        m_favoriteChannels.remove(channel);
        // Clean up messages for this channel
        deleteMessagesForChannel(channel);
    } else {
        m_favoriteChannels.insert(channel);
    }
    
    saveSettings();
    emit favoriteChannelsChanged(m_favoriteChannels);
    
    return !wasFavorite; // Return new state
}

bool MessageRetentionService::isFavoriteChannel(const QString& channel) const
{
    return m_favoriteChannels.contains(channel);
}

void MessageRetentionService::addFavoriteChannel(const QString& channel)
{
    if (!channel.isEmpty() && !m_favoriteChannels.contains(channel)) {
        m_favoriteChannels.insert(channel);
        saveSettings();
        emit favoriteChannelsChanged(m_favoriteChannels);
    }
}

void MessageRetentionService::removeFavoriteChannel(const QString& channel)
{
    if (m_favoriteChannels.contains(channel)) {
        m_favoriteChannels.remove(channel);
        deleteMessagesForChannel(channel);
        saveSettings();
        emit favoriteChannelsChanged(m_favoriteChannels);
    }
}

void MessageRetentionService::saveMessage(const BitchatMessage& message, const QString& channel)
{
    QString channelName = channel.isEmpty() ? QString::fromStdString(message.channel) : channel;
    
    // Only save messages for favorite channels
    if (channelName.isEmpty() || !m_favoriteChannels.contains(channelName)) {
        return;
    }
    
    // Convert to StoredMessage
    StoredMessage storedMessage;
    storedMessage.id = QString::fromStdString(message.id);
    storedMessage.sender = QString::fromStdString(message.sender);
    storedMessage.senderPeerID = QString::fromStdString(message.senderPeerID);
    storedMessage.content = QString::fromStdString(message.content);
    storedMessage.timestamp = QDateTime::fromSecsSinceEpoch(message.timestamp);
    storedMessage.channelTag = QString::fromStdString(message.channel);
    storedMessage.isPrivate = message.isPrivate;
    storedMessage.recipientPeerID = QString::fromStdString(message.senderPeerID); // Note: This might need adjustment based on actual BitchatMessage API
    
    // Serialize to JSON
    QJsonObject jsonObj;
    jsonObj["id"] = storedMessage.id;
    jsonObj["sender"] = storedMessage.sender;
    jsonObj["senderPeerID"] = storedMessage.senderPeerID;
    jsonObj["content"] = storedMessage.content;
    jsonObj["timestamp"] = storedMessage.timestamp.toString(Qt::ISODate);
    jsonObj["channelTag"] = storedMessage.channelTag;
    jsonObj["isPrivate"] = storedMessage.isPrivate;
    jsonObj["recipientPeerID"] = storedMessage.recipientPeerID;
    
    QJsonDocument jsonDoc(jsonObj);
    QByteArray messageData = jsonDoc.toJson(QJsonDocument::Compact);
    
    // Encrypt message
    QByteArray encryptedData = encryptMessage(messageData);
    if (encryptedData.isEmpty()) {
        qWarning() << "Failed to encrypt message for storage";
        return;
    }
    
    // Save to file
    QString filePath = getMessageFilePath(channelName, storedMessage.id, storedMessage.timestamp);
    if (saveEncryptedMessageToFile(encryptedData, filePath)) {
        qDebug() << "Saved message to storage:" << storedMessage.id << "channel:" << channelName;
        emit messagesSaved(channelName, 1);
    } else {
        qWarning() << "Failed to save message to storage:" << storedMessage.id;
    }
}

std::vector<BitchatMessage> MessageRetentionService::loadMessagesForChannel(const QString& channel) const
{
    std::vector<BitchatMessage> messages;
    
    if (channel.isEmpty() || !m_favoriteChannels.contains(channel)) {
        return messages;
    }
    
    QString channelDir = m_storageDirectory + "/" + channel;
    QDir dir(channelDir);
    if (!dir.exists()) {
        return messages;
    }
    
    QStringList filters;
    filters << "*.enc";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    for (const QFileInfo& fileInfo : files) {
        QByteArray encryptedData = loadEncryptedMessageFromFile(fileInfo.absoluteFilePath());
        if (encryptedData.isEmpty()) {
            continue;
        }
        
        QByteArray decryptedData = decryptMessage(encryptedData);
        if (decryptedData.isEmpty()) {
            continue;
        }
        
        QJsonDocument jsonDoc = QJsonDocument::fromJson(decryptedData);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            continue;
        }
        
        QJsonObject jsonObj = jsonDoc.object();
        
        // Create BitchatMessage from stored data
        BitchatMessage message;
        message.id = jsonObj["id"].toString().toStdString();
        message.sender = jsonObj["sender"].toString().toStdString();
        message.content = jsonObj["content"].toString().toStdString();
        message.timestamp = QDateTime::fromString(jsonObj["timestamp"].toString(), Qt::ISODate).toSecsSinceEpoch();
        message.isRelay = false;
        message.originalSender = "";
        message.isPrivate = jsonObj["isPrivate"].toBool();
        message.recipientNickname = "";
        message.senderPeerID = jsonObj["senderPeerID"].toString().toStdString();
        message.mentions = std::vector<std::string>();
        message.channel = jsonObj["channelTag"].toString().toStdString();
        message.encryptedContent = std::vector<uint8_t>();
        message.isEncrypted = false;
        message.deliveryStatus = DeliveryStatus::DELIVERED;
        
        messages.push_back(message);
    }
    
    // Sort messages by timestamp
    std::sort(messages.begin(), messages.end(), [](const BitchatMessage& a, const BitchatMessage& b) {
        return a.timestamp < b.timestamp;
    });
    
    return messages;
}

void MessageRetentionService::deleteMessagesForChannel(const QString& channel)
{
    if (channel.isEmpty()) {
        return;
    }
    
    QString channelDir = m_storageDirectory + "/" + channel;
    QDir dir(channelDir);
    if (!dir.exists()) {
        return;
    }
    
    int deletedCount = 0;
    QStringList filters;
    filters << "*.enc";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo& fileInfo : files) {
        if (QFile::remove(fileInfo.absoluteFilePath())) {
            deletedCount++;
        }
    }
    
    // Remove directory if empty
    if (dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        dir.rmdir(channelDir);
    }
    
    if (deletedCount > 0) {
        qDebug() << "Deleted" << deletedCount << "messages for channel:" << channel;
        emit messagesDeleted(channel, deletedCount);
    }
}

void MessageRetentionService::deleteAllStoredMessages()
{
    QDir storageDir(m_storageDirectory);
    if (!storageDir.exists()) {
        return;
    }
    
    int totalDeleted = 0;
    QStringList channelDirs = storageDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& channelDir : channelDirs) {
        QString channelPath = m_storageDirectory + "/" + channelDir;
        QDir dir(channelPath);
        
        QStringList filters;
        filters << "*.enc";
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        
        for (const QFileInfo& fileInfo : files) {
            if (QFile::remove(fileInfo.absoluteFilePath())) {
                totalDeleted++;
            }
        }
        
        // Remove directory if empty
        if (dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
            dir.rmdir(channelPath);
        }
    }
    
    // Clear favorite channels
    m_favoriteChannels.clear();
    saveSettings();
    emit favoriteChannelsChanged(m_favoriteChannels);
    
    qDebug() << "Deleted all stored messages, total:" << totalDeleted;
}

void MessageRetentionService::setRetentionDays(int days)
{
    if (days > 0) {
        m_retentionDays = days;
        saveSettings();
    }
}

int MessageRetentionService::getRetentionDays() const
{
    return m_retentionDays;
}

int MessageRetentionService::getStoredMessageCount() const
{
    int count = 0;
    
    QDirIterator it(m_storageDirectory, QStringList() << "*.enc", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        count++;
    }
    
    return count;
}

int MessageRetentionService::getStoredMessageCountForChannel(const QString& channel) const
{
    if (channel.isEmpty()) {
        return 0;
    }
    
    QString channelDir = m_storageDirectory + "/" + channel;
    QDir dir(channelDir);
    if (!dir.exists()) {
        return 0;
    }
    
    QStringList filters;
    filters << "*.enc";
    return dir.entryList(filters, QDir::Files).size();
}

QStringList MessageRetentionService::getChannelsWithStoredMessages() const
{
    QDir storageDir(m_storageDirectory);
    if (!storageDir.exists()) {
        return QStringList();
    }
    
    return storageDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

void MessageRetentionService::cleanupOldMessages()
{
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-m_retentionDays);
    int deletedCount = 0;
    
    QDirIterator it(m_storageDirectory, QStringList() << "*.enc", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.birthTime() < cutoffDate || fileInfo.lastModified() < cutoffDate) {
            if (QFile::remove(filePath)) {
                deletedCount++;
            }
        }
    }
    
    if (deletedCount > 0) {
        qDebug() << "Cleaned up" << deletedCount << "old messages";
    }
}

QByteArray MessageRetentionService::encryptMessage(const QByteArray& messageData) const
{
    if (messageData.isEmpty()) {
        return QByteArray();
    }
    
    // Get or create encryption key
    QByteArray keyData = m_keychain->getIdentityKey("messageRetentionKey");
    if (keyData.isEmpty()) {
        // Generate new key
        keyData.resize(crypto_secretbox_KEYBYTES);
        randombytes_buf(keyData.data(), keyData.size());
        
        if (!m_keychain->saveIdentityKey(keyData, "messageRetentionKey")) {
            qWarning() << "Failed to save message retention key";
            return QByteArray();
        }
    }
    
    if (keyData.size() != crypto_secretbox_KEYBYTES) {
        qWarning() << "Invalid key size for message encryption";
        return QByteArray();
    }
    
    // Generate random nonce
    QByteArray nonce(crypto_secretbox_NONCEBYTES, 0);
    randombytes_buf(nonce.data(), nonce.size());
    
    // Encrypt the data
    QByteArray ciphertext(messageData.size() + crypto_secretbox_MACBYTES, 0);
    
    if (crypto_secretbox_easy(reinterpret_cast<unsigned char*>(ciphertext.data()),
                             reinterpret_cast<const unsigned char*>(messageData.data()),
                             messageData.size(),
                             reinterpret_cast<const unsigned char*>(nonce.data()),
                             reinterpret_cast<const unsigned char*>(keyData.data())) != 0) {
        qWarning() << "Failed to encrypt message data";
        return QByteArray();
    }
    
    // Prepend nonce to ciphertext
    return nonce + ciphertext;
}

QByteArray MessageRetentionService::decryptMessage(const QByteArray& encryptedData) const
{
    if (encryptedData.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
        qWarning() << "Invalid encrypted message data size";
        return QByteArray();
    }
    
    // Get encryption key
    QByteArray keyData = m_keychain->getIdentityKey("messageRetentionKey");
    if (keyData.size() != crypto_secretbox_KEYBYTES) {
        qWarning() << "Invalid key size for message decryption";
        return QByteArray();
    }
    
    // Extract nonce and ciphertext
    QByteArray nonce = encryptedData.left(crypto_secretbox_NONCEBYTES);
    QByteArray ciphertext = encryptedData.mid(crypto_secretbox_NONCEBYTES);
    
    // Decrypt the data
    QByteArray plaintext(ciphertext.size() - crypto_secretbox_MACBYTES, 0);
    
    if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char*>(plaintext.data()),
                                  reinterpret_cast<const unsigned char*>(ciphertext.data()),
                                  ciphertext.size(),
                                  reinterpret_cast<const unsigned char*>(nonce.data()),
                                  reinterpret_cast<const unsigned char*>(keyData.data())) != 0) {
        qWarning() << "Failed to decrypt message data";
        return QByteArray();
    }
    
    return plaintext;
}

QString MessageRetentionService::getStorageDirectory() const
{
    return m_storageDirectory;
}

QString MessageRetentionService::getMessageFilePath(const QString& channel, const QString& messageId, const QDateTime& timestamp) const
{
    QString channelDir = m_storageDirectory + "/" + channel;
    QDir dir;
    dir.mkpath(channelDir);
    
    QString fileName = QString("%1_%2_%3.enc")
                       .arg(channel)
                       .arg(timestamp.toSecsSinceEpoch())
                       .arg(messageId);
    
    return channelDir + "/" + fileName;
}

bool MessageRetentionService::saveEncryptedMessageToFile(const QByteArray& encryptedData, const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    qint64 written = file.write(encryptedData);
    file.close();
    
    if (written != encryptedData.size()) {
        qWarning() << "Failed to write complete encrypted data to file:" << filePath;
        return false;
    }
    
    // Set restrictive permissions
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    
    return true;
}

QByteArray MessageRetentionService::loadEncryptedMessageFromFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    return data;
}

void MessageRetentionService::loadSettings()
{
    QSettings settings;
    
    // Load favorite channels
    QStringList favoritesList = settings.value(FAVORITE_CHANNELS_KEY).toStringList();
    m_favoriteChannels = QSet<QString>(favoritesList.begin(), favoritesList.end());
    
    // Load retention days
    m_retentionDays = settings.value(RETENTION_DAYS_KEY, DEFAULT_RETENTION_DAYS).toInt();
    
    qDebug() << "Loaded settings: favorite channels =" << m_favoriteChannels.size()
             << "retention days =" << m_retentionDays;
}

void MessageRetentionService::saveSettings()
{
    QSettings settings;
    
    // Save favorite channels
    QStringList favoritesList(m_favoriteChannels.begin(), m_favoriteChannels.end());
    settings.setValue(FAVORITE_CHANNELS_KEY, favoritesList);
    
    // Save retention days
    settings.setValue(RETENTION_DAYS_KEY, m_retentionDays);
    
    settings.sync();
} 