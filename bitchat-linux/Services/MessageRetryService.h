#pragma once

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QDateTime>
#include <QString>
#include <QMap>
#include <vector>
#include <memory>

class BluetoothMeshService;

struct RetryableMessage {
    QString id;
    QString originalMessageID;
    QDateTime originalTimestamp;
    QString content;
    QStringList mentions;
    QString channel;
    bool isPrivate;
    QString recipientPeerID;
    QString recipientNickname;
    std::vector<uint8_t> channelKey;
    int retryCount;
    int maxRetries;
    QDateTime nextRetryTime;
    
    RetryableMessage() : isPrivate(false), retryCount(0), maxRetries(3) {}
};

class MessageRetryService : public QObject
{
    Q_OBJECT

public:
    static MessageRetryService* shared();
    
    void setMeshService(BluetoothMeshService* meshService);
    
    void addMessageForRetry(
        const QString& content,
        const QStringList& mentions = QStringList(),
        const QString& channel = QString(),
        bool isPrivate = false,
        const QString& recipientPeerID = QString(),
        const QString& recipientNickname = QString(),
        const std::vector<uint8_t>& channelKey = std::vector<uint8_t>(),
        const QString& originalMessageID = QString(),
        const QDateTime& originalTimestamp = QDateTime()
    );
    
    void removeMessage(const QString& messageId);
    void clearQueue();
    int getQueueSize() const;
    
    // Configuration
    void setRetryInterval(int seconds);
    void setMaxRetries(int maxRetries);
    void setMaxQueueSize(int maxSize);
    
    int getRetryInterval() const;
    int getMaxRetries() const;
    int getMaxQueueSize() const;

signals:
    void messageRetryAttempted(const QString& messageId, int retryCount);
    void messageRetryFailed(const QString& messageId, const QString& reason);
    void queueSizeChanged(int newSize);

private slots:
    void processRetryQueue();

private:
    explicit MessageRetryService(QObject* parent = nullptr);
    ~MessageRetryService();
    
    static MessageRetryService* s_instance;
    
    void scheduleNextRetry(const RetryableMessage& message);
    bool shouldRetryMessage(const RetryableMessage& message) const;
    void attemptRetry(const RetryableMessage& message);
    
    BluetoothMeshService* m_meshService;
    QQueue<RetryableMessage> m_retryQueue;
    QTimer* m_retryTimer;
    
    // Configuration
    int m_retryInterval; // seconds
    int m_maxRetries;
    int m_maxQueueSize;
    
    // State
    bool m_processing;
    void handleRetryTimeout(const QString& messageID);
    void cleanupExpiredRetries();
};