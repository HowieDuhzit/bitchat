#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QTimer>
#include <QSet>
#include <vector>
#include <memory>

class KeychainManager;

struct StoredMessage {
    QString id;
    QString sender;
    QString senderPeerID;
    QString content;
    QDateTime timestamp;
    QString channelTag;
    bool isPrivate;
    QString recipientPeerID;
    
    StoredMessage() : isPrivate(false) {}
};

// Forward declaration for BitchatMessage
class BitchatMessage;

class MessageRetentionService : public QObject
{
    Q_OBJECT

public:
    static MessageRetentionService* shared();
    
    // Favorite channels management
    QSet<QString> getFavoriteChannels() const;
    bool toggleFavoriteChannel(const QString& channel);
    bool isFavoriteChannel(const QString& channel) const;
    void addFavoriteChannel(const QString& channel);
    void removeFavoriteChannel(const QString& channel);
    
    // Message storage
    void saveMessage(const BitchatMessage& message, const QString& channel = QString());
    std::vector<BitchatMessage> loadMessagesForChannel(const QString& channel) const;
    
    // Cleanup operations
    void deleteMessagesForChannel(const QString& channel);
    void deleteAllStoredMessages();
    
    // Configuration
    void setRetentionDays(int days);
    int getRetentionDays() const;
    
    // Statistics
    int getStoredMessageCount() const;
    int getStoredMessageCountForChannel(const QString& channel) const;
    QStringList getChannelsWithStoredMessages() const;

signals:
    void favoriteChannelsChanged(const QSet<QString>& channels);
    void messagesSaved(const QString& channel, int count);
    void messagesDeleted(const QString& channel, int count);

private slots:
    void cleanupOldMessages();

private:
    explicit MessageRetentionService(QObject* parent = nullptr);
    ~MessageRetentionService();
    
    static MessageRetentionService* s_instance;
    
    // Encryption helpers
    QByteArray encryptMessage(const QByteArray& messageData) const;
    QByteArray decryptMessage(const QByteArray& encryptedData) const;
    
    // File operations
    QString getStorageDirectory() const;
    QString getMessageFilePath(const QString& channel, const QString& messageId, const QDateTime& timestamp) const;
    bool saveEncryptedMessageToFile(const QByteArray& encryptedData, const QString& filePath) const;
    QByteArray loadEncryptedMessageFromFile(const QString& filePath) const;
    
    // Settings management
    void loadSettings();
    void saveSettings();
    
    QSet<QString> m_favoriteChannels;
    int m_retentionDays;
    QTimer* m_cleanupTimer;
    QString m_storageDirectory;
    KeychainManager* m_keychain;
    
    static const QString FAVORITE_CHANNELS_KEY;
    static const QString RETENTION_DAYS_KEY;
    static const int DEFAULT_RETENTION_DAYS = 7;
};