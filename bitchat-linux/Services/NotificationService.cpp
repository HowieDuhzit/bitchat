#include "NotificationService.h"
#include <QDebug>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>
#include <iostream>

// Disable libnotify for now to avoid gdk-pixbuf dependency issues
// #ifdef signals
// #undef signals
// #define SIGNALS_REDEFINED
// #endif

// #include <libnotify/notify.h>

// #ifdef SIGNALS_REDEFINED
// #define signals Q_SIGNALS
// #undef SIGNALS_REDEFINED
// #endif

NotificationService::NotificationService(QObject *parent)
    : QObject(parent)
    , m_isInitialized(false)
    , m_soundEnabled(true)
    , m_vibrationEnabled(true)
    , m_badgeEnabled(true)
    , m_quietHours(false)
    , m_lastNotificationTime(0)
{
    // Initialize without libnotify for now
    m_isInitialized = true;
}

NotificationService::~NotificationService() {
    // No cleanup needed
}

bool NotificationService::initialize() {
    if (m_isInitialized) {
        return true;
    }
    
    // Simple initialization without libnotify
    m_isInitialized = true;
    return true;
}

void NotificationService::shutdown() {
    m_isInitialized = false;
}

void NotificationService::showNotification(const QString& title, const QString& message, 
                                          NotificationService::NotificationType type) {
    if (!m_isInitialized) {
        return;
    }
    
    // Use simple console output and desktop notification alternatives
    qDebug() << "Notification:" << title << "-" << message;
    
    // Try to use desktop notification via notify-send if available
    QProcess::startDetached("notify-send", QStringList() << title << message);
    
    // For terminal users, also output to stderr
    std::cerr << "📱 " << title.toStdString() << ": " << message.toStdString() << std::endl;
}

void NotificationService::showMessageNotification(const QString& sender, const QString& message, 
                                                 const QString& channel) {
    if (!m_isInitialized) {
        return;
    }
    
    QString title = sender;
    if (!channel.isEmpty()) {
        title += " in #" + channel;
    }
    
    showNotification(title, message, NotificationService::NotificationType::Message);
}

void NotificationService::showSystemNotification(const QString& message) {
    if (!m_isInitialized) {
        return;
    }
    
    showNotification("BitChat", message, NotificationService::NotificationType::System);
}

void NotificationService::showDeliveryNotification(const QString& messageId, const QString& status) {
    if (!m_isInitialized) {
        return;
    }
    
    QString message = QString("Message %1: %2").arg(messageId.left(8)).arg(status);
    showNotification("Delivery Status", message, NotificationService::NotificationType::Delivery);
}

void NotificationService::clearNotifications() {
    // Nothing to clear in simple implementation
}

void NotificationService::setSoundEnabled(bool enabled) {
    m_soundEnabled = enabled;
}

void NotificationService::setVibrationEnabled(bool enabled) {
    m_vibrationEnabled = enabled;
}

void NotificationService::setBadgeEnabled(bool enabled) {
    m_badgeEnabled = enabled;
}

void NotificationService::setQuietHours(bool enabled) {
    m_quietHours = enabled;
}

bool NotificationService::isSoundEnabled() const {
    return m_soundEnabled;
}

bool NotificationService::isVibrationEnabled() const {
    return m_vibrationEnabled;
}

bool NotificationService::isBadgeEnabled() const {
    return m_badgeEnabled;
}

bool NotificationService::isQuietHours() const {
    return m_quietHours;
}

void NotificationService::requestPermission() {
    // No permission needed for simple implementation
    emit permissionGranted();
}

void NotificationService::updateBadgeCount(int count) {
    Q_UNUSED(count)
    // Badge count not supported in simple implementation
}

void NotificationService::playNotificationSound() {
    if (!m_soundEnabled) {
        return;
    }
    
    // Try to play a simple system beep
    QProcess::startDetached("paplay", QStringList() << "/usr/share/sounds/alsa/Front_Left.wav");
}

void NotificationService::triggerVibration() {
    if (!m_vibrationEnabled) {
        return;
    }
    
    // Vibration not supported on desktop Linux
} 