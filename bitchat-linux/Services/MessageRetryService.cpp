#include "MessageRetryService.h"
#include "BluetoothMeshService.h"
#include <QDebug>
#include <QUuid>
#include <algorithm>

MessageRetryService* MessageRetryService::s_instance = nullptr;

MessageRetryService::MessageRetryService(QObject* parent)
    : QObject(parent)
    , m_meshService(nullptr)
    , m_retryTimer(nullptr)
    , m_retryInterval(2) // 2 seconds for faster sync
    , m_maxRetries(3)
    , m_maxQueueSize(50)
    , m_processing(false)
{
    // Initialize retry timer
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(false);
    m_retryTimer->setInterval(m_retryInterval * 1000);
    connect(m_retryTimer, &QTimer::timeout, this, &MessageRetryService::processRetryQueue);
    m_retryTimer->start();
}

MessageRetryService::~MessageRetryService()
{
    if (m_retryTimer) {
        m_retryTimer->stop();
    }
}

MessageRetryService* MessageRetryService::shared()
{
    if (!s_instance) {
        s_instance = new MessageRetryService();
    }
    return s_instance;
}

void MessageRetryService::setMeshService(BluetoothMeshService* meshService)
{
    m_meshService = meshService;
}

void MessageRetryService::addMessageForRetry(
    const QString& content,
    const QStringList& mentions,
    const QString& channel,
    bool isPrivate,
    const QString& recipientPeerID,
    const QString& recipientNickname,
    const std::vector<uint8_t>& channelKey,
    const QString& originalMessageID,
    const QDateTime& originalTimestamp)
{
    // Don't queue empty or whitespace-only messages
    if (content.trimmed().isEmpty()) {
        return;
    }
    
    // Don't queue if we're at capacity
    if (m_retryQueue.size() >= m_maxQueueSize) {
        qWarning() << "Retry queue at capacity, dropping message";
        return;
    }
    
    // Check if this message is already in the queue
    if (!originalMessageID.isEmpty()) {
        for (const auto& msg : m_retryQueue) {
            if (msg.originalMessageID == originalMessageID) {
                return; // Don't add duplicate
            }
        }
    }
    
    RetryableMessage retryMessage;
    retryMessage.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    retryMessage.originalMessageID = originalMessageID;
    retryMessage.originalTimestamp = originalTimestamp.isValid() ? originalTimestamp : QDateTime::currentDateTime();
    retryMessage.content = content;
    retryMessage.mentions = mentions;
    retryMessage.channel = channel;
    retryMessage.isPrivate = isPrivate;
    retryMessage.recipientPeerID = recipientPeerID;
    retryMessage.recipientNickname = recipientNickname;
    retryMessage.channelKey = channelKey;
    retryMessage.retryCount = 0;
    retryMessage.maxRetries = m_maxRetries;
    retryMessage.nextRetryTime = QDateTime::currentDateTime().addSecs(m_retryInterval);
    
    m_retryQueue.enqueue(retryMessage);
    
    // Sort the queue by original timestamp to maintain message order
    std::sort(m_retryQueue.begin(), m_retryQueue.end(), 
              [](const RetryableMessage& a, const RetryableMessage& b) {
                  return a.originalTimestamp < b.originalTimestamp;
              });
    
    emit queueSizeChanged(m_retryQueue.size());
    
    qDebug() << "Added message to retry queue:" << retryMessage.id 
             << "content:" << content.left(50) << "..." 
             << "queue size:" << m_retryQueue.size();
}

void MessageRetryService::removeMessage(const QString& messageId)
{
    for (int i = 0; i < m_retryQueue.size(); ++i) {
        if (m_retryQueue[i].id == messageId || m_retryQueue[i].originalMessageID == messageId) {
            m_retryQueue.removeAt(i);
            emit queueSizeChanged(m_retryQueue.size());
            qDebug() << "Removed message from retry queue:" << messageId;
            return;
        }
    }
}

void MessageRetryService::clearQueue()
{
    int oldSize = m_retryQueue.size();
    m_retryQueue.clear();
    
    if (oldSize > 0) {
        emit queueSizeChanged(0);
        qDebug() << "Cleared retry queue, removed" << oldSize << "messages";
    }
}

int MessageRetryService::getQueueSize() const
{
    return m_retryQueue.size();
}

void MessageRetryService::setRetryInterval(int seconds)
{
    m_retryInterval = seconds;
    if (m_retryTimer) {
        m_retryTimer->setInterval(seconds * 1000);
    }
}

void MessageRetryService::setMaxRetries(int maxRetries)
{
    m_maxRetries = maxRetries;
}

void MessageRetryService::setMaxQueueSize(int maxSize)
{
    m_maxQueueSize = maxSize;
    
    // Enforce new limit
    while (m_retryQueue.size() > m_maxQueueSize) {
        m_retryQueue.dequeue();
    }
    
    emit queueSizeChanged(m_retryQueue.size());
}

int MessageRetryService::getRetryInterval() const
{
    return m_retryInterval;
}

int MessageRetryService::getMaxRetries() const
{
    return m_maxRetries;
}

int MessageRetryService::getMaxQueueSize() const
{
    return m_maxQueueSize;
}

void MessageRetryService::processRetryQueue()
{
    if (m_processing || m_retryQueue.isEmpty() || !m_meshService) {
        return;
    }
    
    m_processing = true;
    
    QDateTime now = QDateTime::currentDateTime();
    QQueue<RetryableMessage> tempQueue;
    
    // Process up to 5 messages per cycle to avoid blocking
    int processed = 0;
    const int maxProcessPerCycle = 5;
    
    while (!m_retryQueue.isEmpty() && processed < maxProcessPerCycle) {
        RetryableMessage message = m_retryQueue.dequeue();
        
        if (now >= message.nextRetryTime) {
            if (shouldRetryMessage(message)) {
                attemptRetry(message);
                message.retryCount++;
                message.nextRetryTime = now.addSecs(m_retryInterval * (1 << message.retryCount)); // Exponential backoff
                tempQueue.enqueue(message);
                
                emit messageRetryAttempted(message.id, message.retryCount);
                processed++;
            } else {
                // Max retries exceeded
                emit messageRetryFailed(message.id, "Max retries exceeded");
                qDebug() << "Message retry failed permanently:" << message.id;
            }
        } else {
            // Not time to retry yet
            tempQueue.enqueue(message);
        }
    }
    
    // Re-add remaining messages to the queue
    while (!tempQueue.isEmpty()) {
        m_retryQueue.enqueue(tempQueue.dequeue());
    }
    
    emit queueSizeChanged(m_retryQueue.size());
    m_processing = false;
}

void MessageRetryService::scheduleNextRetry(const RetryableMessage& message)
{
    // This is handled in processRetryQueue with exponential backoff
    Q_UNUSED(message)
}

bool MessageRetryService::shouldRetryMessage(const RetryableMessage& message) const
{
    return message.retryCount < message.maxRetries;
}

void MessageRetryService::attemptRetry(const RetryableMessage& message)
{
    if (!m_meshService) {
        qWarning() << "No mesh service available for retry";
        return;
    }
    
    qDebug() << "Attempting retry" << (message.retryCount + 1) << "for message:" << message.id;
    
    if (message.isPrivate) {
        m_meshService->sendPrivateMessage(message.content.toStdString(), message.recipientPeerID.toStdString());
    } else if (!message.channel.isEmpty()) {
        m_meshService->sendMessage(message.content.toStdString(), message.channel.toStdString());
    } else {
        m_meshService->sendMessage(message.content.toStdString());
    }
} 