#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <memory>

class BitchatApplication;
class ChatWidget;
class PeerListWidget;
class ChannelWidget;
class SettingsDialog;
struct BitchatMessage;
struct PeerInfo;
struct ChannelInfo;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(BitchatApplication* app, QWidget *parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    
private slots:
    void onSendMessage();
    void onMessageReceived(const BitchatMessage& message);
    void onMessageSent(const BitchatMessage& message);
    void onPeerConnected(const PeerInfo& peer);
    void onPeerDisconnected(const PeerInfo& peer);
    void onChannelJoined(const ChannelInfo& channel);
    void onChannelLeft(const std::string& channel);
    void onConnectionStatusChanged(bool connected);
    void onNotificationReceived(const std::string& title, const std::string& body);
    
    // Menu actions
    void onJoinChannel();
    void onLeaveChannel();
    void onSettings();
    void onAbout();
    void onQuit();
    
    // System tray actions
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowWindow();
    void onHideWindow();
    
    // UI updates
    void updateConnectionStatus();
    void updatePeerCount();
    void updateChannelList();
    void updateStatusBar();
    
private:
    // Setup methods
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupSystemTray();
    void setupConnections();
    void applyTheme();
    
    // Helper methods
    QString formatMessage(const BitchatMessage& message) const;
    QString formatTimestamp(uint64_t timestamp) const;
    QString formatPeerCount(int count) const;
    QString formatConnectionStatus(bool connected) const;
    void showMessage(const QString& title, const QString& message);
    void showNotification(const QString& title, const QString& message);

private:
    // Core application
    BitchatApplication* m_app;
    
    // UI components
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    QSplitter* m_leftSplitter;
    
    // Chat area
    ChatWidget* m_chatWidget;
    QLineEdit* m_messageInput;
    QPushButton* m_sendButton;
    
    // Side panels
    PeerListWidget* m_peerListWidget;
    ChannelWidget* m_channelWidget;
    
    // Status bar
    QLabel* m_connectionStatusLabel;
    QLabel* m_peerCountLabel;
    QLabel* m_currentChannelLabel;
    
    // Menu bar
    QMenu* m_fileMenu;
    QMenu* m_channelMenu;
    QMenu* m_toolsMenu;
    QMenu* m_helpMenu;
    
    // Actions
    QAction* m_joinChannelAction;
    QAction* m_leaveChannelAction;
    QAction* m_settingsAction;
    QAction* m_quitAction;
    QAction* m_aboutAction;
    
    // System tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    QAction* m_showAction;
    QAction* m_hideAction;
    QAction* m_trayQuitAction;
    
    // Dialogs
    SettingsDialog* m_settingsDialog;
    
    // State
    bool m_isConnected;
    int m_peerCount;
    std::string m_currentChannel;
}; 