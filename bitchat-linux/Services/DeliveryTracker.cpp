#include "DeliveryTracker.h"
#include <QDebug>
#include <QTimer>
#include <cmath>

DeliveryTracker* DeliveryTracker::s_instance = nullptr;

bool PendingDelivery::isTimedOut() const
{
    int timeout = isFavorite ? 300 : (isChannelMessage ? 60 : 30);
    return QDateTime::currentDateTime().secsTo(sentAt) > timeout;
}

bool PendingDelivery::shouldRetry() const
{
    return retryCount < 3 && isFavorite && !isChannelMessage;
}

DeliveryTracker::DeliveryTracker(QObject* parent)
    : QObject(parent)
    , m_privateMessageTimeout(30)    // 30 seconds
    , m_roomMessageTimeout(60)       // 1 minute
    , m_favoriteTimeout(300)         // 5 minutes
    , m_maxRetries(3)
    , m_retryDelay(5)               // 5 seconds base delay
    , m_cleanupTimer(nullptr)
{
    // Set up cleanup timer (run every 5 minutes)
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setSingleShot(false);
    m_cleanupTimer->setInterval(5 * 60 * 1000); // 5 minutes
    connect(m_cleanupTimer, &QTimer::timeout, this, &DeliveryTracker::cleanupOldDeliveries);
    m_cleanupTimer->start();
}

DeliveryTracker::~DeliveryTracker()
{
    // Clean up all timers
    for (auto it = m_pendingDeliveries.begin(); it != m_pendingDeliveries.end(); ++it) {
        if (it.value().timeoutTimer) {
            it.value().timeoutTimer->stop();
            delete it.value().timeoutTimer;
        }
    }
    
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
    }
}

DeliveryTracker* DeliveryTracker::shared()
{
    if (!s_instance) {
        s_instance = new DeliveryTracker();
    }
    return s_instance;
}

void DeliveryTracker::trackMessage(const QString& messageID, const QString& recipientID, 
                                  const QString& recipientNickname, bool isChannelMessage, 
                                   bool isFavorite, int expectedRecipients) {
    if (messageID.isEmpty() || recipientID.isEmpty()) {
        return;
    }
    
    PendingDelivery delivery;
    delivery.messageID = messageID;
    delivery.sentAt = QDateTime::currentDateTime();
    delivery.recipientID = recipientID;
    delivery.recipientNickname = recipientNickname;
    delivery.isChannelMessage = isChannelMessage;
    delivery.isFavorite = isFavorite;
    delivery.expectedRecipients = expectedRecipients;
    
    m_pendingDeliveries[messageID] = delivery;
    
    // Schedule timeout
    scheduleTimeout(messageID);
    
    qDebug() << "Tracking message:" << messageID << "for recipient:" << recipientID;
    
    emit deliveryStatusUpdated(messageID, DeliveryStatus::SENDING);
}

void DeliveryTracker::handleDeliveryAck(const DeliveryAck& ack) {
    if (ack.originalMessageID.empty() || ack.recipientID.empty()) {
        return;
    }
    
    QString ackKey = QString::fromStdString(ack.originalMessageID) + ":" + QString::fromStdString(ack.recipientID);
    
    // Prevent duplicate ACKs
    if (m_receivedAckIDs.contains(ackKey)) {
        return;
    }
    
    m_receivedAckIDs.insert(ackKey);
    
    auto it = m_pendingDeliveries.find(QString::fromStdString(ack.originalMessageID));
    if (it == m_pendingDeliveries.end()) {
        qDebug() << "Received ACK for non-tracked message:" << QString::fromStdString(ack.originalMessageID);
        return;
    }
    
    PendingDelivery& delivery = it.value();
    delivery.ackedBy.insert(QString::fromStdString(ack.recipientID));
    
    qDebug() << "Received delivery ACK:" << QString::fromStdString(ack.originalMessageID)
             << "from:" << QString::fromStdString(ack.recipientID)
             << "acked by:" << delivery.ackedBy.size() << "/" << delivery.expectedRecipients;
    
    emit deliveryAckReceived(QString::fromStdString(ack.originalMessageID), QString::fromStdString(ack.recipientID));
    
    // Check if all expected recipients have acknowledged
    if (delivery.ackedBy.size() >= delivery.expectedRecipients) {
        updateDeliveryStatus(QString::fromStdString(ack.originalMessageID), DeliveryStatus::DELIVERED);
    }
}

void DeliveryTracker::handleReadReceipt(const ReadReceipt& receipt) {
    if (receipt.originalMessageID.empty() || receipt.readerID.empty()) {
        return;
    }
    
    qDebug() << "Received read receipt:" << QString::fromStdString(receipt.originalMessageID)
             << "from:" << QString::fromStdString(receipt.readerID);
    
    emit readReceiptReceived(QString::fromStdString(receipt.originalMessageID), QString::fromStdString(receipt.readerID));
    
    // Update delivery status to READ
    updateDeliveryStatus(QString::fromStdString(receipt.originalMessageID), DeliveryStatus::READ);
}

DeliveryStatus DeliveryTracker::getDeliveryStatus(const QString& messageID) const {
    if (m_pendingDeliveries.contains(messageID)) {
        return DeliveryStatus::SENDING;
    }
    
    return DeliveryStatus::DELIVERED;
}

bool DeliveryTracker::isMessagePending(const QString& messageID) const
{
    return m_pendingDeliveries.contains(messageID);
}

QStringList DeliveryTracker::getPendingMessages() const
{
    return m_pendingDeliveries.keys();
}

void DeliveryTracker::setPrivateMessageTimeout(int seconds)
{
    m_privateMessageTimeout = seconds;
}

void DeliveryTracker::setRoomMessageTimeout(int seconds)
{
    m_roomMessageTimeout = seconds;
}

void DeliveryTracker::setFavoriteTimeout(int seconds)
{
    m_favoriteTimeout = seconds;
}

void DeliveryTracker::setMaxRetries(int maxRetries)
{
    m_maxRetries = maxRetries;
}

void DeliveryTracker::setRetryDelay(int seconds)
{
    m_retryDelay = seconds;
}

int DeliveryTracker::getPrivateMessageTimeout() const
{
    return m_privateMessageTimeout;
}

int DeliveryTracker::getRoomMessageTimeout() const
{
    return m_roomMessageTimeout;
}

int DeliveryTracker::getFavoriteTimeout() const
{
    return m_favoriteTimeout;
}

int DeliveryTracker::getMaxRetries() const
{
    return m_maxRetries;
}

int DeliveryTracker::getRetryDelay() const
{
    return m_retryDelay;
}

void DeliveryTracker::removeMessage(const QString& messageID)
{
    auto it = m_pendingDeliveries.find(messageID);
    if (it != m_pendingDeliveries.end()) {
        if (it.value().timeoutTimer) {
            it.value().timeoutTimer->stop();
            delete it.value().timeoutTimer;
        }
        m_pendingDeliveries.erase(it);
        
        qDebug() << "Removed message from tracking:" << messageID;
    }
}

void DeliveryTracker::clearAllPendingMessages()
{
    for (auto it = m_pendingDeliveries.begin(); it != m_pendingDeliveries.end(); ++it) {
        if (it.value().timeoutTimer) {
            it.value().timeoutTimer->stop();
            delete it.value().timeoutTimer;
        }
    }
    
    int count = m_pendingDeliveries.size();
    m_pendingDeliveries.clear();
    m_receivedAckIDs.clear();
    m_sentAckIDs.clear();
    
    qDebug() << "Cleared all pending deliveries, count:" << count;
}

void DeliveryTracker::handleTimeout()
{
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) {
        return;
    }
    
    // Find the message associated with this timer
    QString messageID;
    for (auto it = m_pendingDeliveries.begin(); it != m_pendingDeliveries.end(); ++it) {
        if (it.value().timeoutTimer == timer) {
            messageID = it.key();
            break;
        }
    }
    
    if (!messageID.isEmpty()) {
        handleTimeoutForMessage(messageID);
    }
}

void DeliveryTracker::cleanupOldDeliveries()
{
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-3600); // 1 hour ago
    QStringList toRemove;
    
    for (auto it = m_pendingDeliveries.begin(); it != m_pendingDeliveries.end(); ++it) {
        if (it.value().sentAt < cutoff) {
            toRemove.append(it.key());
        }
    }
    
    for (const QString& messageID : toRemove) {
        removeMessage(messageID);
        updateDeliveryStatus(messageID, DeliveryStatus::FAILED);
    }
    
    if (!toRemove.isEmpty()) {
        qDebug() << "Cleaned up" << toRemove.size() << "old deliveries";
    }
    
    // Clean up ACK sets if they get too large
    if (m_receivedAckIDs.size() > 1000) {
        m_receivedAckIDs.clear();
    }
    if (m_sentAckIDs.size() > 1000) {
        m_sentAckIDs.clear();
    }
}

void DeliveryTracker::scheduleTimeout(const QString& messageID)
{
    auto it = m_pendingDeliveries.find(messageID);
    if (it == m_pendingDeliveries.end()) {
        return;
    }
    
    PendingDelivery& delivery = it.value();
    
    int timeout = delivery.isFavorite ? m_favoriteTimeout :
                 (delivery.isChannelMessage ? m_roomMessageTimeout : m_privateMessageTimeout);
    
    delivery.timeoutTimer = new QTimer(this);
    delivery.timeoutTimer->setSingleShot(true);
    delivery.timeoutTimer->setInterval(timeout * 1000);
    connect(delivery.timeoutTimer, &QTimer::timeout, this, &DeliveryTracker::handleTimeout);
    delivery.timeoutTimer->start();
    
    qDebug() << "Scheduled timeout for message:" << messageID 
             << "timeout:" << timeout << "seconds";
}

void DeliveryTracker::handleTimeoutForMessage(const QString& messageID)
{
    auto it = m_pendingDeliveries.find(messageID);
    if (it == m_pendingDeliveries.end()) {
        return;
    }
    
    PendingDelivery& delivery = it.value();
    
    qDebug() << "Timeout for message:" << messageID 
             << "retryCount:" << delivery.retryCount
             << "shouldRetry:" << delivery.shouldRetry();
    
    if (delivery.shouldRetry()) {
        // Retry the message
        delivery.retryCount++;
        retryDelivery(messageID);
        
        // Schedule new timeout with exponential backoff
        int newTimeout = delivery.isFavorite ? m_favoriteTimeout :
                        (delivery.isChannelMessage ? m_roomMessageTimeout : m_privateMessageTimeout);
        newTimeout *= std::pow(2, delivery.retryCount - 1); // Exponential backoff
        
        if (delivery.timeoutTimer) {
            delivery.timeoutTimer->stop();
            delete delivery.timeoutTimer;
        }
        
        delivery.timeoutTimer = new QTimer(this);
        delivery.timeoutTimer->setSingleShot(true);
        delivery.timeoutTimer->setInterval(newTimeout * 1000);
        connect(delivery.timeoutTimer, &QTimer::timeout, this, &DeliveryTracker::handleTimeout);
        delivery.timeoutTimer->start();
        
        qDebug() << "Retrying message:" << messageID 
                 << "attempt:" << delivery.retryCount
                 << "nextTimeout:" << newTimeout;
    } else {
        // Mark as failed
        updateDeliveryStatus(messageID, DeliveryStatus::FAILED);
        
        qDebug() << "Message delivery failed:" << messageID << "reason:" << "Message not delivered";
    }
}

void DeliveryTracker::retryDelivery(const QString& messageID)
{
    emit messageRetryRequested(messageID);
}

void DeliveryTracker::updateDeliveryStatus(const QString& messageID, DeliveryStatus status)
{
    emit deliveryStatusUpdated(messageID, status);
} 