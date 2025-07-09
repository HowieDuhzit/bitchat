#include "MainWindow.h"
#include "../../BitchatApplication.h"
#include "../../ViewModels/ConfigManager.h"
#include "../../Services/BluetoothMeshService.h"
#include "../../ViewModels/MessageHandler.h"
#include <QApplication>
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
#include <QCloseEvent>
#include <QEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QScrollBar>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QFontMetrics>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QKeyEvent>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QCheckBox>
#include <memory>

// Custom widget for chat display
class ChatWidget : public QTextEdit {
    Q_OBJECT
public:
    explicit ChatWidget(QWidget* parent = nullptr) : QTextEdit(parent) {
        setReadOnly(true);
        setFont(QFont("Consolas", 10));
        
        // Dark theme styling
        setStyleSheet(R"(
            QTextEdit {
                background-color: #1e1e1e;
                color: #00ff00;
                border: 1px solid #333333;
                selection-background-color: #404040;
            }
        )");
        
        // Auto-scroll to bottom
        connect(this, &QTextEdit::textChanged, this, &ChatWidget::scrollToBottom);
    }
    
    void appendMessage(const QString& message) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(message + "\n");
        setTextCursor(cursor);
        scrollToBottom();
    }
    
private slots:
    void scrollToBottom() {
        QScrollBar* scrollBar = verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }
};

// Custom widget for peer list
class PeerListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit PeerListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setMaximumWidth(200);
        setFont(QFont("Consolas", 9));
        
        // Dark theme styling
        setStyleSheet(R"(
            QListWidget {
                background-color: #2d2d2d;
                color: #00ff00;
                border: 1px solid #333333;
                selection-background-color: #404040;
            }
            QListWidget::item {
                padding: 5px;
                border-bottom: 1px solid #333333;
            }
            QListWidget::item:selected {
                background-color: #404040;
            }
        )");
        
        // Context menu
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QListWidget::customContextMenuRequested, this, &PeerListWidget::showContextMenu);
    }
    
    void updatePeers(const QStringList& peers) {
        clear();
        for (const QString& peer : peers) {
            addItem(QString("● %1").arg(peer));
        }
    }
    
private slots:
    void showContextMenu(const QPoint& pos) {
        QListWidgetItem* item = itemAt(pos);
        if (!item) return;
        
        QMenu contextMenu(this);
        contextMenu.addAction("Send Private Message", [this, item]() {
            emit privateMessageRequested(item->text().mid(2)); // Remove "● " prefix
        });
        contextMenu.addAction("Block User", [this, item]() {
            emit blockUserRequested(item->text().mid(2));
        });
        contextMenu.exec(mapToGlobal(pos));
    }
    
signals:
    void privateMessageRequested(const QString& peer);
    void blockUserRequested(const QString& peer);
};

// Custom widget for channel list
class ChannelWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit ChannelWidget(QWidget* parent = nullptr) : QTreeWidget(parent) {
        setMaximumWidth(200);
        setFont(QFont("Consolas", 9));
        setHeaderHidden(true);
        
        // Dark theme styling
        setStyleSheet(R"(
            QTreeWidget {
                background-color: #2d2d2d;
                color: #00ff00;
                border: 1px solid #333333;
                selection-background-color: #404040;
            }
            QTreeWidget::item {
                padding: 3px;
                border-bottom: 1px solid #333333;
            }
            QTreeWidget::item:selected {
                background-color: #404040;
            }
        )");
        
        // Add default items
        m_channelsRoot = new QTreeWidgetItem(this, QStringList("Channels"));
        m_channelsRoot->setExpanded(true);
        
        // Context menu
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QTreeWidget::customContextMenuRequested, this, &ChannelWidget::showContextMenu);
        connect(this, &QTreeWidget::itemClicked, this, &ChannelWidget::onItemClicked);
    }
    
    void updateChannels(const QStringList& channels) {
        // Clear existing channels
        while (m_channelsRoot->childCount() > 0) {
            delete m_channelsRoot->takeChild(0);
        }
        
        // Add channels
        for (const QString& channel : channels) {
            QTreeWidgetItem* item = new QTreeWidgetItem(m_channelsRoot, QStringList(QString("#%1").arg(channel)));
            item->setData(0, Qt::UserRole, channel);
        }
    }
    
private slots:
    void showContextMenu(const QPoint& pos) {
        QTreeWidgetItem* item = itemAt(pos);
        if (!item || item == m_channelsRoot) return;
        
        QMenu contextMenu(this);
        contextMenu.addAction("Join Channel", [this, item]() {
            emit joinChannelRequested(item->data(0, Qt::UserRole).toString());
        });
        contextMenu.addAction("Leave Channel", [this, item]() {
            emit leaveChannelRequested(item->data(0, Qt::UserRole).toString());
        });
        contextMenu.exec(mapToGlobal(pos));
    }
    
    void onItemClicked(QTreeWidgetItem* item, int column) {
        if (item && item != m_channelsRoot) {
            emit channelSelected(item->data(0, Qt::UserRole).toString());
        }
    }
    
signals:
    void joinChannelRequested(const QString& channel);
    void leaveChannelRequested(const QString& channel);
    void channelSelected(const QString& channel);
    
private:
    QTreeWidgetItem* m_channelsRoot;
};

// Settings dialog
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(BitchatApplication* app, QWidget* parent = nullptr) 
        : QDialog(parent), m_app(app) {
        setWindowTitle("Settings");
        setModal(true);
        resize(400, 300);
        
        // Dark theme styling
        setStyleSheet(R"(
            QDialog {
                background-color: #1e1e1e;
                color: #00ff00;
            }
            QLabel {
                color: #00ff00;
            }
            QLineEdit {
                background-color: #2d2d2d;
                color: #00ff00;
                border: 1px solid #333333;
                padding: 5px;
            }
            QCheckBox {
                color: #00ff00;
            }
            QPushButton {
                background-color: #2d2d2d;
                color: #00ff00;
                border: 1px solid #333333;
                padding: 5px 10px;
            }
            QPushButton:hover {
                background-color: #404040;
            }
        )");
        
        setupUI();
    }
    
private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // Nickname setting
        layout->addWidget(new QLabel("Nickname:"));
        m_nicknameEdit = new QLineEdit(this);
        layout->addWidget(m_nicknameEdit);
        
        // Notification settings
        m_notificationsEnabled = new QCheckBox("Enable notifications", this);
        layout->addWidget(m_notificationsEnabled);
        
        // Encryption settings
        m_encryptionEnabled = new QCheckBox("Enable encryption", this);
        layout->addWidget(m_encryptionEnabled);
        
        // Battery optimization
        m_batteryOptimization = new QCheckBox("Enable battery optimization", this);
        layout->addWidget(m_batteryOptimization);
        
        // Buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("OK", this);
        QPushButton* cancelButton = new QPushButton("Cancel", this);
        
        connect(okButton, &QPushButton::clicked, this, &SettingsDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &SettingsDialog::reject);
        
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);
        
        layout->addLayout(buttonLayout);
        
        loadSettings();
    }
    
    void loadSettings() {
        ConfigManager* config = m_app->getConfigManager();
        if (config) {
            m_nicknameEdit->setText(QString::fromStdString(config->getNickname()));
            m_notificationsEnabled->setChecked(config->areNotificationsEnabled());
            m_encryptionEnabled->setChecked(config->isEncryptionEnabled());
            m_batteryOptimization->setChecked(config->isBatteryOptimizationEnabled());
        }
    }
    
    void saveSettings() {
        ConfigManager* config = m_app->getConfigManager();
        if (config) {
            config->setNickname(m_nicknameEdit->text().toStdString());
            config->setNotificationsEnabled(m_notificationsEnabled->isChecked());
            config->setEncryptionEnabled(m_encryptionEnabled->isChecked());
            config->setBatteryOptimizationEnabled(m_batteryOptimization->isChecked());
            config->save();
        }
    }
    
    void accept() override {
        saveSettings();
        QDialog::accept();
    }
    
private:
    BitchatApplication* m_app;
    QLineEdit* m_nicknameEdit;
    QCheckBox* m_notificationsEnabled;
    QCheckBox* m_encryptionEnabled;
    QCheckBox* m_batteryOptimization;
};

// Main Window Implementation
MainWindow::MainWindow(BitchatApplication* app, QWidget *parent)
    : QMainWindow(parent)
    , m_app(app)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_leftSplitter(nullptr)
    , m_chatWidget(nullptr)
    , m_messageInput(nullptr)
    , m_sendButton(nullptr)
    , m_peerListWidget(nullptr)
    , m_channelWidget(nullptr)
    , m_connectionStatusLabel(nullptr)
    , m_peerCountLabel(nullptr)
    , m_currentChannelLabel(nullptr)
    , m_trayIcon(nullptr)
    , m_settingsDialog(nullptr)
    , m_isConnected(false)
    , m_peerCount(0)
    , m_currentChannel("general")
{
    setWindowTitle("BitChat - Decentralized Mesh Messaging");
    setWindowIcon(QIcon(":/icons/bitchat.png")); // TODO: Add icon resource
    resize(1000, 700);
    
    // Dark theme styling
    setStyleSheet(R"(
        QMainWindow {
            background-color: #1e1e1e;
            color: #00ff00;
        }
        QStatusBar {
            background-color: #2d2d2d;
            color: #00ff00;
            border-top: 1px solid #333333;
        }
        QMenuBar {
            background-color: #2d2d2d;
            color: #00ff00;
            border-bottom: 1px solid #333333;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 5px 10px;
        }
        QMenuBar::item:selected {
            background-color: #404040;
        }
        QMenu {
            background-color: #2d2d2d;
            color: #00ff00;
            border: 1px solid #333333;
        }
        QMenu::item {
            padding: 5px 20px;
        }
        QMenu::item:selected {
            background-color: #404040;
        }
        QSplitter::handle {
            background-color: #333333;
        }
    )");
    
    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupSystemTray();
    setupConnections();
    
    // Initialize state
    updateConnectionStatus();
    updatePeerCount();
    updateChannelList();
    updateStatusBar();
}

MainWindow::~MainWindow() {
    delete m_settingsDialog;
}

void MainWindow::setupUI() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Main splitter (horizontal)
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Left splitter (vertical)
    m_leftSplitter = new QSplitter(Qt::Vertical, this);
    
    // Channel widget
    m_channelWidget = new ChannelWidget(this);
    m_leftSplitter->addWidget(m_channelWidget);
    
    // Peer list widget
    m_peerListWidget = new PeerListWidget(this);
    m_leftSplitter->addWidget(m_peerListWidget);
    
    // Set left splitter sizes
    m_leftSplitter->setSizes({300, 300});
    
    // Chat area
    QWidget* chatArea = new QWidget(this);
    QVBoxLayout* chatLayout = new QVBoxLayout(chatArea);
    chatLayout->setContentsMargins(5, 5, 5, 5);
    
    // Chat widget
    m_chatWidget = new ChatWidget(this);
    chatLayout->addWidget(m_chatWidget);
    
    // Input area
    QHBoxLayout* inputLayout = new QHBoxLayout();
    
    m_messageInput = new QLineEdit(this);
    m_messageInput->setPlaceholderText("Type a message... (use /help for commands)");
    m_messageInput->setFont(QFont("Consolas", 10));
    m_messageInput->setStyleSheet(R"(
        QLineEdit {
            background-color: #2d2d2d;
            color: #00ff00;
            border: 1px solid #333333;
            padding: 8px;
            border-radius: 4px;
        }
        QLineEdit:focus {
            border: 1px solid #00ff00;
        }
    )");
    
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setFont(QFont("Consolas", 10));
    m_sendButton->setStyleSheet(R"(
        QPushButton {
            background-color: #2d2d2d;
            color: #00ff00;
            border: 1px solid #333333;
            padding: 8px 16px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #404040;
        }
        QPushButton:pressed {
            background-color: #1e1e1e;
        }
    )");
    
    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendButton);
    
    chatLayout->addLayout(inputLayout);
    
    // Add to main splitter
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(chatArea);
    
    // Set main splitter sizes
    m_mainSplitter->setSizes({250, 750});
    
    mainLayout->addWidget(m_mainSplitter);
    
    // Connect input signals
    connect(m_messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendMessage);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendMessage);
    
    // Connect widget signals
    connect(m_peerListWidget, &PeerListWidget::privateMessageRequested, 
            this, [this](const QString& peer) {
                m_messageInput->setText(QString("/msg %1 ").arg(peer));
                m_messageInput->setFocus();
            });
    
    connect(m_channelWidget, &ChannelWidget::joinChannelRequested,
            this, [this](const QString& channel) {
                m_messageInput->setText(QString("/join %1").arg(channel));
                onSendMessage();
            });
    
    connect(m_channelWidget, &ChannelWidget::leaveChannelRequested,
            this, [this](const QString& channel) {
                m_messageInput->setText(QString("/leave %1").arg(channel));
                onSendMessage();
            });
    
    connect(m_channelWidget, &ChannelWidget::channelSelected,
            this, [this](const QString& channel) {
                m_currentChannel = channel.toStdString();
                updateStatusBar();
            });
}

void MainWindow::setupMenuBar() {
    m_fileMenu = menuBar()->addMenu("&File");
    m_channelMenu = menuBar()->addMenu("&Channel");
    m_toolsMenu = menuBar()->addMenu("&Tools");
    m_helpMenu = menuBar()->addMenu("&Help");
    
    // File menu
    m_quitAction = m_fileMenu->addAction("&Quit");
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::onQuit);
    
    // Channel menu
    m_joinChannelAction = m_channelMenu->addAction("&Join Channel...");
    m_joinChannelAction->setShortcut(QKeySequence("Ctrl+J"));
    connect(m_joinChannelAction, &QAction::triggered, this, &MainWindow::onJoinChannel);
    
    m_leaveChannelAction = m_channelMenu->addAction("&Leave Channel");
    m_leaveChannelAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(m_leaveChannelAction, &QAction::triggered, this, &MainWindow::onLeaveChannel);
    
    // Tools menu
    m_settingsAction = m_toolsMenu->addAction("&Settings...");
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    
    // Help menu
    m_aboutAction = m_helpMenu->addAction("&About BitChat");
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupStatusBar() {
    m_connectionStatusLabel = new QLabel("Disconnected", this);
    m_connectionStatusLabel->setStyleSheet("color: #ff0000;");
    statusBar()->addWidget(m_connectionStatusLabel);
    
    statusBar()->addPermanentWidget(new QLabel(" | "));
    
    m_peerCountLabel = new QLabel("0 peers", this);
    statusBar()->addPermanentWidget(m_peerCountLabel);
    
    statusBar()->addPermanentWidget(new QLabel(" | "));
    
    m_currentChannelLabel = new QLabel("#general", this);
    statusBar()->addPermanentWidget(m_currentChannelLabel);
}

void MainWindow::setupSystemTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/bitchat.png")); // TODO: Add icon resource
    m_trayIcon->setToolTip("BitChat - Decentralized Mesh Messaging");
    
    // Tray menu
    m_trayMenu = new QMenu(this);
    
    m_showAction = m_trayMenu->addAction("Show");
    connect(m_showAction, &QAction::triggered, this, &MainWindow::onShowWindow);
    
    m_hideAction = m_trayMenu->addAction("Hide");
    connect(m_hideAction, &QAction::triggered, this, &MainWindow::onHideWindow);
    
    m_trayMenu->addSeparator();
    
    m_trayQuitAction = m_trayMenu->addAction("Quit");
    connect(m_trayQuitAction, &QAction::triggered, this, &MainWindow::onQuit);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    
    m_trayIcon->show();
}

void MainWindow::setupConnections() {
    // Connect to application signals
    connect(m_app, &BitchatApplication::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_app, &BitchatApplication::messageSent, this, &MainWindow::onMessageSent);
    connect(m_app, &BitchatApplication::peerConnected, this, &MainWindow::onPeerConnected);
    connect(m_app, &BitchatApplication::peerDisconnected, this, &MainWindow::onPeerDisconnected);
    connect(m_app, &BitchatApplication::channelJoined, this, &MainWindow::onChannelJoined);
    connect(m_app, &BitchatApplication::channelLeft, this, &MainWindow::onChannelLeft);
    connect(m_app, &BitchatApplication::connectionStatusChanged, this, &MainWindow::onConnectionStatusChanged);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
        showMessage("BitChat", "Application was minimized to tray");
    } else {
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_trayIcon && m_trayIcon->isVisible()) {
            hide();
            event->ignore();
        }
    }
}

// Slots implementation
void MainWindow::onSendMessage() {
    QString message = m_messageInput->text().trimmed();
    if (message.isEmpty()) return;
    
    // Send message through application
    m_app->sendMessage(message.toStdString());
    
    // Clear input
    m_messageInput->clear();
    m_messageInput->setFocus();
}

void MainWindow::onMessageReceived(const BitchatMessage& message) {
    QString formattedMessage = formatMessage(message);
    m_chatWidget->appendMessage(formattedMessage);
    
    // Show notification if window is not active
    if (!isActiveWindow()) {
        showNotification("New Message", QString::fromStdString(message.content));
    }
}

void MainWindow::onMessageSent(const BitchatMessage& message) {
    QString formattedMessage = formatMessage(message);
    m_chatWidget->appendMessage(formattedMessage);
}

void MainWindow::onPeerConnected(const PeerInfo& peer) {
    updatePeerCount();
    updateConnectionStatus();
    showMessage("Peer Connected", QString("Connected to %1").arg(QString::fromStdString(peer.nickname)));
}

void MainWindow::onPeerDisconnected(const PeerInfo& peer) {
    updatePeerCount();
    updateConnectionStatus();
    showMessage("Peer Disconnected", QString("Disconnected from %1").arg(QString::fromStdString(peer.nickname)));
}

void MainWindow::onChannelJoined(const ChannelInfo& channel) {
    updateChannelList();
    showMessage("Channel Joined", QString("Joined channel #%1").arg(QString::fromStdString(channel.name)));
}

void MainWindow::onChannelLeft(const std::string& channel) {
    updateChannelList();
    showMessage("Channel Left", QString("Left channel #%1").arg(QString::fromStdString(channel)));
}

void MainWindow::onConnectionStatusChanged(bool connected) {
    m_isConnected = connected;
    updateConnectionStatus();
}

void MainWindow::onNotificationReceived(const std::string& title, const std::string& body) {
    showNotification(QString::fromStdString(title), QString::fromStdString(body));
}

// Menu actions
void MainWindow::onJoinChannel() {
    bool ok;
    QString channel = QInputDialog::getText(this, "Join Channel", "Channel name:", QLineEdit::Normal, "", &ok);
    if (ok && !channel.isEmpty()) {
        m_app->sendMessage(QString("/join %1").arg(channel).toStdString());
    }
}

void MainWindow::onLeaveChannel() {
    if (m_currentChannel.empty()) return;
    
    int ret = QMessageBox::question(this, "Leave Channel", 
                                   QString("Are you sure you want to leave #%1?").arg(QString::fromStdString(m_currentChannel)),
                                   QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_app->sendMessage(QString("/leave %1").arg(QString::fromStdString(m_currentChannel)).toStdString());
    }
}

void MainWindow::onSettings() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(m_app, this);
    }
    m_settingsDialog->exec();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About BitChat", 
                      "BitChat v1.0.0\n\n"
                      "Decentralized mesh messaging over Bluetooth LE\n\n"
                      "Features:\n"
                      "• No internet required\n"
                      "• End-to-end encryption\n"
                      "• Peer-to-peer mesh networking\n"
                      "• Channel-based communication\n"
                      "• Store & forward messaging\n\n"
                      "This software is released into the public domain.");
}

void MainWindow::onQuit() {
    m_app->shutdown();
    QApplication::quit();
}

// System tray actions
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
        break;
    default:
        break;
    }
}

void MainWindow::onShowWindow() {
    show();
    raise();
    activateWindow();
}

void MainWindow::onHideWindow() {
    hide();
}

// UI updates
void MainWindow::updateConnectionStatus() {
    if (m_isConnected) {
        m_connectionStatusLabel->setText("Connected");
        m_connectionStatusLabel->setStyleSheet("color: #00ff00;");
    } else {
        m_connectionStatusLabel->setText("Disconnected");
        m_connectionStatusLabel->setStyleSheet("color: #ff0000;");
    }
}

void MainWindow::updatePeerCount() {
    // Get peer count from application
    m_peerCount = m_app->getPeerCount();
    m_peerCountLabel->setText(formatPeerCount(m_peerCount));
    
    // Update peer list
    QStringList peers = m_app->getPeerList();
    m_peerListWidget->updatePeers(peers);
}

void MainWindow::updateChannelList() {
    // Get channel list from application
    QStringList channels = m_app->getChannelList();
    m_channelWidget->updateChannels(channels);
}

void MainWindow::updateStatusBar() {
    m_currentChannelLabel->setText(QString("#%1").arg(QString::fromStdString(m_currentChannel)));
}

// Helper methods
QString MainWindow::formatMessage(const BitchatMessage& message) const {
    QString timestamp = formatTimestamp(message.timestamp);
    QString sender = QString::fromStdString(message.sender);
    QString content = QString::fromStdString(message.content);
    
    if (!message.channel.empty()) {
        // This is a channel message
        return QString("[%1] <%2> %3").arg(timestamp, sender, content);
    } else if (message.isPrivate) {
        // This is a private message
        return QString("[%1] *%2* %3").arg(timestamp, sender, content);
    } else {
        // This is a system or other message
        return QString("[%1] %2").arg(timestamp, content);
    }
}

QString MainWindow::formatTimestamp(uint64_t timestamp) const {
    QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(timestamp);
    return dateTime.toString("hh:mm:ss");
}

QString MainWindow::formatPeerCount(int count) const {
    return QString("%1 peer%2").arg(count).arg(count == 1 ? "" : "s");
}

QString MainWindow::formatConnectionStatus(bool connected) const {
    return connected ? "Connected" : "Disconnected";
}

void MainWindow::showMessage(const QString& title, const QString& message) {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 3000);
    }
}

void MainWindow::showNotification(const QString& title, const QString& message) {
    showMessage(title, message);
}

#include "MainWindow.moc" 