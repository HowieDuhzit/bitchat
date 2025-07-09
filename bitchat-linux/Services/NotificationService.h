#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class NotificationService : public QObject {
    Q_OBJECT
    
public:
    enum class NotificationType {
        Message,
        System,
        Delivery,
        Error
    };
    
    explicit NotificationService(QObject* parent = nullptr);
    ~NotificationService();
    
    // Core functionality
    bool initialize();
    void shutdown();
    
    // Notification display
    void showNotification(const QString& title, const QString& message, NotificationType type = NotificationType::Message);
    void showMessageNotification(const QString& sender, const QString& message, const QString& channel = QString());
    void showSystemNotification(const QString& message);
    void showDeliveryNotification(const QString& messageId, const QString& status);
    void clearNotifications();
    
    // Settings
    void setSoundEnabled(bool enabled);
    void setVibrationEnabled(bool enabled);
    void setBadgeEnabled(bool enabled);
    void setQuietHours(bool enabled);
    
    bool isSoundEnabled() const;
    bool isVibrationEnabled() const;
    bool isBadgeEnabled() const;
    bool isQuietHours() const;
    
    // Permissions and badge
    void requestPermission();
    void updateBadgeCount(int count);
    
signals:
    void permissionGranted();
    void permissionDenied();
    void notificationClicked(const QString& actionId);
    void notificationClosed(const QString& notificationId);
    
private slots:
    void playNotificationSound();
    void triggerVibration();
    
private:
    bool m_isInitialized;
    bool m_soundEnabled;
    bool m_vibrationEnabled;
    bool m_badgeEnabled;
    bool m_quietHours;
    qint64 m_lastNotificationTime;
}; 