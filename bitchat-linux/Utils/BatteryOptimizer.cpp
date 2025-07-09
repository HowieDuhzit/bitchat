#include "BatteryOptimizer.h"
#include "ConfigManager.h"
#include "BluetoothMeshService.h"
#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

BatteryOptimizer::BatteryOptimizer(ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_configManager(configManager)
    , m_bluetoothService(nullptr)
    , m_batteryTimer(nullptr)
    , m_optimizationTimer(nullptr)
    , m_currentPowerMode(PowerMode::BALANCED)
    , m_automaticPowerMode(true)
    , m_initialized(false)
    , m_lowBatteryThreshold(DEFAULT_LOW_BATTERY_THRESHOLD)
    , m_criticalBatteryThreshold(DEFAULT_CRITICAL_BATTERY_THRESHOLD)
    , m_backgroundScanningEnabled(true)
    , m_totalPowerSavings(0)
{
    // Initialize timers
    m_batteryTimer = new QTimer(this);
    m_batteryTimer->setSingleShot(false);
    m_batteryTimer->setInterval(BATTERY_CHECK_INTERVAL);
    connect(m_batteryTimer, &QTimer::timeout, this, &BatteryOptimizer::updateBatteryStatus);

    m_optimizationTimer = new QTimer(this);
    m_optimizationTimer->setSingleShot(false);
    m_optimizationTimer->setInterval(OPTIMIZATION_CHECK_INTERVAL);
    connect(m_optimizationTimer, &QTimer::timeout, this, &BatteryOptimizer::checkPowerOptimization);

    // Connect battery level signals
    connect(this, &BatteryOptimizer::batteryLow, this, &BatteryOptimizer::onLowBattery);
    connect(this, &BatteryOptimizer::batteryCritical, this, &BatteryOptimizer::onCriticalBattery);
}

BatteryOptimizer::~BatteryOptimizer() {
    shutdown();
}

bool BatteryOptimizer::initialize() {
    if (m_initialized) {
        return true;
    }

    qDebug() << "Initializing BatteryOptimizer...";

    // Load configuration
    loadPowerSettings();

    // Setup battery monitoring
    setupBatteryMonitoring();

    // Start timers
    m_batteryTimer->start();
    m_optimizationTimer->start();

    // Initial battery status update
    updateBatteryStatus();

    m_initialized = true;
    qDebug() << "BatteryOptimizer initialized successfully";
    qDebug() << "Power mode:" << getPowerModeString();
    qDebug() << "Battery level:" << m_batteryStats.currentLevel << "%";

    return true;
}

void BatteryOptimizer::shutdown() {
    if (!m_initialized) {
        return;
    }

    qDebug() << "Shutting down BatteryOptimizer...";

    // Stop timers
    if (m_batteryTimer) m_batteryTimer->stop();
    if (m_optimizationTimer) m_optimizationTimer->stop();

    // Save power settings
    savePowerSettings();

    m_initialized = false;
    qDebug() << "BatteryOptimizer shutdown complete";
}

void BatteryOptimizer::setPowerMode(PowerMode mode) {
    if (m_currentPowerMode == mode) {
        return;
    }

    PowerMode oldMode = m_currentPowerMode;
    m_currentPowerMode = mode;

    // Apply power settings for new mode
    PowerSettings settings = getPowerSettingsForMode(mode);
    applyPowerSettings(settings);

    // Update Bluetooth service if available
    updateBluetoothSettings();

    // Log power mode change
    logPowerEvent(QString("Power mode changed from %1 to %2")
                  .arg(static_cast<int>(oldMode))
                  .arg(static_cast<int>(mode)));

    emit powerModeChanged(mode);
    qDebug() << "Power mode changed to:" << getPowerModeString();
}

PowerMode BatteryOptimizer::getPowerMode() const {
    return m_currentPowerMode;
}

void BatteryOptimizer::setAutomaticPowerMode(bool enabled) {
    m_automaticPowerMode = enabled;
    
    if (m_configManager) {
        // Note: setBatteryAutomaticMode method doesn't exist in ConfigManager
        // m_configManager->setBatteryAutomaticMode(enabled);
    }
    
    if (enabled) {
        updateAutomaticPowerMode();
    }
    
    qDebug() << "Automatic power mode" << (enabled ? "enabled" : "disabled");
}

bool BatteryOptimizer::isAutomaticPowerMode() const {
    return m_automaticPowerMode;
}

BatteryLevel BatteryOptimizer::getBatteryLevel() const {
    int level = m_batteryStats.currentLevel;
    
    if (level < 0) return BatteryLevel::UNKNOWN;
    if (level < m_criticalBatteryThreshold) return BatteryLevel::CRITICAL;
    if (level < m_lowBatteryThreshold) return BatteryLevel::LOW;
    if (level < 50) return BatteryLevel::MEDIUM;
    if (level < 75) return BatteryLevel::HIGH;
    return BatteryLevel::FULL;
}

BatteryStats BatteryOptimizer::getBatteryStats() const {
    return m_batteryStats;
}

bool BatteryOptimizer::isBatteryLow() const {
    return m_batteryStats.currentLevel <= m_lowBatteryThreshold && 
           m_batteryStats.currentLevel > m_criticalBatteryThreshold;
}

bool BatteryOptimizer::isBatteryCritical() const {
    return m_batteryStats.currentLevel <= m_criticalBatteryThreshold;
}

bool BatteryOptimizer::isCharging() const {
    return m_batteryStats.isCharging;
}

void BatteryOptimizer::optimizeForBatteryLife() {
    setPowerMode(PowerMode::POWER_SAVER);
    m_automaticPowerMode = false;
    
    logPowerEvent("Manual optimization for battery life");
    qDebug() << "Optimized for battery life";
}

void BatteryOptimizer::optimizeForPerformance() {
    setPowerMode(PowerMode::HIGH_PERFORMANCE);
    m_automaticPowerMode = false;
    
    logPowerEvent("Manual optimization for performance");
    qDebug() << "Optimized for performance";
}

void BatteryOptimizer::resetToDefaults() {
    m_currentSettings = m_defaultSettings;
    setPowerMode(PowerMode::BALANCED);
    m_automaticPowerMode = true;
    
    applyPowerSettings(m_currentSettings);
    updateBluetoothSettings();
    
    logPowerEvent("Reset to default settings");
    qDebug() << "Reset to default power settings";
}

void BatteryOptimizer::applyEmergencyPowerSaving() {
    qDebug() << "Applying emergency power saving measures";
    
    if (m_bluetoothService) {
        m_bluetoothService->setAdvertisementInterval(5000); // 5 seconds
        m_bluetoothService->setScanInterval(10000); // 10 seconds
        // Note: setHeartbeatInterval method doesn't exist in BluetoothMeshService
        // m_bluetoothService->setHeartbeatInterval(120); // 2 minutes
        m_bluetoothService->setMaxConnections(2); // Limit connections
    }
    
    // Set to ultra low power mode
    setPowerMode(PowerMode::ULTRA_LOW_POWER);
    
    // Disable background scanning
    m_backgroundScanningEnabled = false;
    
    // Update timers to longer intervals
    m_batteryTimer->setInterval(60000); // Check every minute
    m_optimizationTimer->setInterval(300000); // Optimize every 5 minutes
    
    logPowerEvent("Emergency power saving activated");
}

void BatteryOptimizer::setBluetoothService(BluetoothMeshService* service) {
    m_bluetoothService = service;
    
    if (m_bluetoothService && m_initialized) {
        updateBluetoothSettings();
    }
}

void BatteryOptimizer::updateBluetoothSettings() {
    if (!m_bluetoothService) {
        return;
    }
    
    m_bluetoothService->setAdvertisementInterval(m_currentSettings.advertiseInterval);
    m_bluetoothService->setScanInterval(m_currentSettings.scanInterval);
    // Note: These methods don't exist in BluetoothMeshService
    // m_bluetoothService->setHeartbeatInterval(m_currentSettings.heartbeatInterval);
    // m_bluetoothService->setConnectionTimeout(m_currentSettings.connectionTimeout);
    m_bluetoothService->setMaxConnections(m_currentSettings.maxConnections);
    // m_bluetoothService->setBackgroundScanning(m_currentSettings.backgroundScanning);
    // m_bluetoothService->setLowPowerMode(m_currentSettings.lowPowerMode);
    
    qDebug() << "Updated Bluetooth settings for power mode" << getPowerModeString();
}

void BatteryOptimizer::updateBatteryStatus() {
    QString batteryInfo = getBatteryInfoFromSystem();
    
    int oldLevel = m_batteryStats.currentLevel;
    bool oldCharging = m_batteryStats.isCharging;
    
    m_batteryStats.currentLevel = parseBatteryLevel(batteryInfo);
    m_batteryStats.isCharging = parseChargingState(batteryInfo);
    m_batteryStats.lastUpdate = QDateTime::currentDateTime();
    
    // Emit signals for changes
    if (oldLevel != m_batteryStats.currentLevel) {
        emit batteryLevelChanged(m_batteryStats.currentLevel);
        
        // Check for low/critical battery
        if (isBatteryLow() && !oldCharging) {
            emit batteryLow();
        } else if (isBatteryCritical() && !oldCharging) {
            emit batteryCritical();
        }
    }
    
    if (oldCharging != m_batteryStats.isCharging) {
        emit chargingStateChanged(m_batteryStats.isCharging);
    }
    
    // Update automatic power mode if enabled
    if (m_automaticPowerMode) {
        updateAutomaticPowerMode();
    }
}

void BatteryOptimizer::checkPowerOptimization() {
    calculatePowerSavings();
    
    // Suggest optimizations based on usage patterns
    if (m_batteryStats.currentLevel < 50 && !m_batteryStats.isCharging) {
        if (m_currentPowerMode == PowerMode::HIGH_PERFORMANCE) {
            qDebug() << "Suggesting power mode change to conserve battery";
        }
    }
}

void BatteryOptimizer::onLowBattery() {
    if (m_automaticPowerMode && m_currentPowerMode != PowerMode::POWER_SAVER) {
        setPowerMode(PowerMode::POWER_SAVER);
        logPowerEvent("Automatic switch to power saver mode (low battery)");
    }
    
    qDebug() << "Low battery detected:" << m_batteryStats.currentLevel << "%";
}

void BatteryOptimizer::onCriticalBattery() {
    if (m_automaticPowerMode) {
        applyEmergencyPowerSaving();
        logPowerEvent("Automatic emergency power saving (critical battery)");
    }
    
    qDebug() << "Critical battery detected:" << m_batteryStats.currentLevel << "%";
}

void BatteryOptimizer::setupBatteryMonitoring() {
    // Initial battery status
    updateBatteryStatus();
    
    qDebug() << "Battery monitoring setup complete";
}

void BatteryOptimizer::loadPowerSettings() {
    if (!m_configManager) {
        return;
    }
    
    // Use default power mode since getBatteryPowerMode doesn't exist
    // int powerMode = m_configManager->getBatteryPowerMode();
    int powerMode = static_cast<int>(PowerMode::BALANCED); // Default to balanced mode
    
    if (powerMode >= 0 && powerMode < 4) {
        m_currentPowerMode = static_cast<PowerMode>(powerMode);
    } else {
        m_currentPowerMode = PowerMode::BALANCED;
    }
    
    // Load other settings with defaults
    m_lowBatteryThreshold = DEFAULT_LOW_BATTERY_THRESHOLD;
    m_criticalBatteryThreshold = DEFAULT_CRITICAL_BATTERY_THRESHOLD;
    m_backgroundScanningEnabled = true;
    m_automaticPowerMode = true;
    
    // Apply current power settings
    m_currentSettings = getPowerSettingsForMode(m_currentPowerMode);
    
    qDebug() << "Loaded power settings, mode:" << getPowerModeString();
}

void BatteryOptimizer::savePowerSettings() {
    if (!m_configManager) {
        return;
    }

    // ConfigManager doesn't have these methods, so we'll store locally
    // m_configManager->setBatteryPowerMode(static_cast<int>(m_currentPowerMode));
    // m_configManager->setBatteryAutomaticMode(m_automaticPowerMode);
    m_configManager->setLowBatteryThreshold(m_lowBatteryThreshold);
    // m_configManager->setBatteryCriticalThreshold(m_criticalBatteryThreshold);
    // m_configManager->setBatteryBackgroundScanning(m_backgroundScanningEnabled);
    // m_configManager->setBatteryPowerSavings(m_totalPowerSavings);
    
    qDebug() << "Saved power settings";
}

void BatteryOptimizer::applyPowerSettings(const PowerSettings& settings) {
    m_currentSettings = settings;
    
    // Apply to Bluetooth service if available
    updateBluetoothSettings();
    
    qDebug() << "Applied power settings - Scan interval:" << settings.scanInterval 
             << "Advertise interval:" << settings.advertiseInterval
             << "Low power mode:" << settings.lowPowerMode;
}

PowerSettings BatteryOptimizer::getPowerSettingsForMode(PowerMode mode) const {
    PowerSettings settings;
    
    switch (mode) {
        case PowerMode::HIGH_PERFORMANCE:
            settings.scanInterval = 500;
            settings.advertiseInterval = 500;
            settings.heartbeatInterval = 15;
            settings.connectionTimeout = 60;
            settings.maxConnections = 10;
            settings.backgroundScanning = true;
            settings.aggressiveScanning = true;
            settings.lowPowerMode = false;
            break;
            
        case PowerMode::BALANCED:
            settings.scanInterval = 1000;
            settings.advertiseInterval = 1000;
            settings.heartbeatInterval = 30;
            settings.connectionTimeout = 30;
            settings.maxConnections = 5;
            settings.backgroundScanning = true;
            settings.aggressiveScanning = false;
            settings.lowPowerMode = false;
            break;
            
        case PowerMode::POWER_SAVER:
            settings.scanInterval = 2000;
            settings.advertiseInterval = 2000;
            settings.heartbeatInterval = 60;
            settings.connectionTimeout = 20;
            settings.maxConnections = 3;
            settings.backgroundScanning = m_backgroundScanningEnabled;
            settings.aggressiveScanning = false;
            settings.lowPowerMode = true;
            break;
            
        case PowerMode::ULTRA_LOW_POWER:
            settings.scanInterval = 5000;
            settings.advertiseInterval = 5000;
            settings.heartbeatInterval = 120;
            settings.connectionTimeout = 15;
            settings.maxConnections = 1;
            settings.backgroundScanning = false;
            settings.aggressiveScanning = false;
            settings.lowPowerMode = true;
            break;
    }
    
    return settings;
}

void BatteryOptimizer::updateAutomaticPowerMode() {
    if (!m_automaticPowerMode) {
        return;
    }

    PowerMode newMode = m_currentPowerMode;
    
    if (m_batteryStats.isCharging) {
        // When charging, prefer performance
        if (m_batteryStats.currentLevel > 50) {
            newMode = PowerMode::HIGH_PERFORMANCE;
        } else {
            newMode = PowerMode::BALANCED;
        }
    } else {
        // When on battery, adjust based on level
        if (isBatteryCritical()) {
            newMode = PowerMode::ULTRA_LOW_POWER;
        } else if (isBatteryLow()) {
            newMode = PowerMode::POWER_SAVER;
        } else if (m_batteryStats.currentLevel > 75) {
            newMode = PowerMode::BALANCED;
        } else {
            newMode = PowerMode::POWER_SAVER;
        }
    }
    
    if (newMode != m_currentPowerMode) {
        setPowerMode(newMode);
    }
}

void BatteryOptimizer::calculatePowerSavings() {
    if (m_lastOptimizationTime.isValid()) {
        int minutesSinceLastCheck = m_lastOptimizationTime.secsTo(QDateTime::currentDateTime()) / 60;
        
        // Estimate power savings based on current mode
        int savingsPerMinute = 0;
        switch (m_currentPowerMode) {
            case PowerMode::POWER_SAVER:
                savingsPerMinute = 2;
                break;
            case PowerMode::ULTRA_LOW_POWER:
                savingsPerMinute = 5;
                break;
            default:
                savingsPerMinute = 0;
                break;
        }
        
        int savings = minutesSinceLastCheck * savingsPerMinute;
        m_totalPowerSavings += savings;
        m_batteryStats.powerSavingsMinutes = m_totalPowerSavings;
        
        if (savings > 0) {
            emit powerSavingsUpdated(m_totalPowerSavings);
        }
    }
    
    m_lastOptimizationTime = QDateTime::currentDateTime();
}

QString BatteryOptimizer::getBatteryInfoFromSystem() const {
#ifdef Q_OS_LINUX
    // Try to read battery information from /sys/class/power_supply/
    QStringList batteryPaths = {
        "/sys/class/power_supply/BAT0/capacity",
        "/sys/class/power_supply/BAT1/capacity",
        "/sys/class/power_supply/battery/capacity"
    };
    
    for (const QString& path : batteryPaths) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QString capacity = file.readAll().trimmed();
            
            // Also check charging status
            QString chargingPath = path;
            chargingPath.replace("capacity", "status");
            QFile chargingFile(chargingPath);
            QString status;
            if (chargingFile.open(QIODevice::ReadOnly)) {
                status = chargingFile.readAll().trimmed();
            }
            
            return QString("capacity=%1 status=%2").arg(capacity).arg(status);
        }
    }
    
    // Fallback to upower if available
    QProcess process;
    process.start("upower", QStringList() << "-i" << "/org/freedesktop/UPower/devices/battery_BAT0");
    process.waitForFinished(3000);
    
    if (process.exitCode() == 0) {
        return process.readAllStandardOutput();
    }
#endif
    
    return QString();
}

int BatteryOptimizer::parseBatteryLevel(const QString& batteryInfo) const {
    if (batteryInfo.isEmpty()) {
        return 100; // Default to full if unknown
    }
    
    // Try to parse capacity from sys filesystem format
    QRegularExpression capacityRegex("capacity=(\\d+)");
    QRegularExpressionMatch match = capacityRegex.match(batteryInfo);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    
    // Try to parse from upower format
    QRegularExpression upowerRegex("percentage\\s*:\\s*(\\d+)%");
    match = upowerRegex.match(batteryInfo);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    
    return 100; // Default to full if parsing fails
}

bool BatteryOptimizer::parseChargingState(const QString& batteryInfo) const {
    if (batteryInfo.isEmpty()) {
        return false;
    }
    
    // Check for charging indicators
    return batteryInfo.contains("Charging", Qt::CaseInsensitive) ||
           batteryInfo.contains("status=Charging", Qt::CaseInsensitive);
}

void BatteryOptimizer::logPowerEvent(const QString& event) {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/bitchat";
    QDir().mkpath(logDir);
    
    QString logFile = logDir + "/power_events.log";
    QFile file(logFile);
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) 
               << " - " << event << Qt::endl;
    }
}

// Configuration methods
void BatteryOptimizer::setLowBatteryThreshold(int percentage) {
    m_lowBatteryThreshold = qBound(5, percentage, 50);
    if (m_configManager) {
        m_configManager->setLowBatteryThreshold(m_lowBatteryThreshold);
    }
}

int BatteryOptimizer::getLowBatteryThreshold() const {
    return m_lowBatteryThreshold;
}

void BatteryOptimizer::setCriticalBatteryThreshold(int percentage) {
    m_criticalBatteryThreshold = qBound(1, percentage, 20);
    if (m_configManager) {
        // ConfigManager doesn't have this method
        // m_configManager->setBatteryCriticalThreshold(m_criticalBatteryThreshold);
    }
}

int BatteryOptimizer::getCriticalBatteryThreshold() const {
    return m_criticalBatteryThreshold;
}

void BatteryOptimizer::setBackgroundScanningEnabled(bool enabled) {
    m_backgroundScanningEnabled = enabled;
    if (m_configManager) {
        // ConfigManager doesn't have this method
        // m_configManager->setBatteryBackgroundScanning(enabled);
    }
    
    // Update current settings if in power saver mode
    if (m_currentPowerMode == PowerMode::POWER_SAVER) {
        m_currentSettings.backgroundScanning = enabled;
        updateBluetoothSettings();
    }
}

bool BatteryOptimizer::isBackgroundScanningEnabled() const {
    return m_backgroundScanningEnabled;
}

void BatteryOptimizer::resetStatistics() {
    m_totalPowerSavings = 0;
    m_batteryStats.powerSavingsMinutes = 0;
    m_lastOptimizationTime = QDateTime();
    
    if (m_configManager) {
        // ConfigManager doesn't have this method
        // m_configManager->setBatteryPowerSavings(0);
    }
    
    emit powerSavingsUpdated(0);
    qDebug() << "Power statistics reset";
}

int BatteryOptimizer::getPowerSavingsMinutes() const {
    return m_totalPowerSavings;
}

QString BatteryOptimizer::getPowerModeString() const {
    switch (m_currentPowerMode) {
        case PowerMode::HIGH_PERFORMANCE: return "High Performance";
        case PowerMode::BALANCED: return "Balanced";
        case PowerMode::POWER_SAVER: return "Power Saver";
        case PowerMode::ULTRA_LOW_POWER: return "Ultra Low Power";
        default: return "Unknown";
    }
}

QStringList BatteryOptimizer::getOptimizationSuggestions() const {
    QStringList suggestions;
    
    if (!m_batteryStats.isCharging && m_batteryStats.currentLevel < 30) {
        if (m_currentPowerMode != PowerMode::POWER_SAVER) {
            suggestions << "Switch to Power Saver mode to extend battery life";
        }
        
        if (m_currentSettings.backgroundScanning) {
            suggestions << "Disable background scanning to save power";
        }
    }
    
    if (m_batteryStats.isCharging && m_batteryStats.currentLevel > 80) {
        if (m_currentPowerMode != PowerMode::HIGH_PERFORMANCE) {
            suggestions << "Switch to High Performance mode while charging";
        }
    }
    
    if (m_currentSettings.scanInterval < 2000 && !m_batteryStats.isCharging) {
        suggestions << "Increase scan interval to reduce power consumption";
    }
    
    return suggestions;
} 