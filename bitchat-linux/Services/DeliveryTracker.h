#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <memory>
#include "../Protocols/BinaryProtocol.h"

struct PendingDelivery {
    QString messageID;
    QDateTime sentAt;
    QString recipientID;
    QString recipientNickname;
    int retryCount;
    bool isChannelMessage;
    bool isFavorite;
    QSet<QString> ackedBy;
    int expectedRecipients;
    QTimer* timeoutTimer;
    
    PendingDelivery() : retryCount(0), isChannelMessage(false), isFavorite(false), 
                       expectedRecipients(1), timeoutTimer(nullptr) {}
    
    bool isTimedOut() const;
    bool shouldRetry() const;
};

class DeliveryTracker : public QObject
{
    Q_OBJECT

public:
    static DeliveryTracker* shared();
    
    // Delivery tracking
    void trackMessage(const QString& messageID, const QString& recipientID, 
                     const QString& recipientNickname = QString(), 
                     bool isChannelMessage = false, bool isFavorite = false,
                     int expectedRecipients = 1);
    
    void handleDeliveryAck(const DeliveryAck& ack);
    void handleReadReceipt(const ReadReceipt& receipt);
    
    // Status queries
    DeliveryStatus getDeliveryStatus(const QString& messageID) const;
    bool isMessagePending(const QString& messageID) const;
    QStringList getPendingMessages() const;
    
    // Configuration
    void setPrivateMessageTimeout(int seconds);
    void setRoomMessageTimeout(int seconds);
    void setFavoriteTimeout(int seconds);
    void setMaxRetries(int maxRetries);
    void setRetryDelay(int seconds);
    
    int getPrivateMessageTimeout() const;
    int getRoomMessageTimeout() const;
    int getFavoriteTimeout() const;
    int getMaxRetries() const;
    int getRetryDelay() const;
    
    // Cleanup
    void removeMessage(const QString& messageID);
    void clearAllPendingMessages();

signals:
    void deliveryStatusUpdated(const QString& messageID, DeliveryStatus status);
    void messageRetryRequested(const QString& messageID);
    void deliveryAckReceived(const QString& messageID, const QString& recipientID);
    void readReceiptReceived(const QString& messageID, const QString& recipientID);

private slots:
    void handleTimeout();
    void cleanupOldDeliveries();

private:
    explicit DeliveryTracker(QObject* parent = nullptr);
    ~DeliveryTracker();
    
    static DeliveryTracker* s_instance;
    
    void scheduleTimeout(const QString& messageID);
    void handleTimeoutForMessage(const QString& messageID);
    void retryDelivery(const QString& messageID);
    void updateDeliveryStatus(const QString& messageID, DeliveryStatus status);
    
    QMap<QString, PendingDelivery> m_pendingDeliveries;
    QSet<QString> m_receivedAckIDs;
    QSet<QString> m_sentAckIDs;
    
    // Configuration
    int m_privateMessageTimeout;  // seconds
    int m_roomMessageTimeout;     // seconds
    int m_favoriteTimeout;        // seconds
    int m_maxRetries;
    int m_retryDelay;            // seconds
    
    // Cleanup timer
    QTimer* m_cleanupTimer;
};