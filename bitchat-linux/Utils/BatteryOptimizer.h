#ifndef BATTERYOPTIMIZER_H
#define BATTERYOPTIMIZER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDateTime>

class ConfigManager;
class BluetoothMeshService;

enum class PowerMode {
    HIGH_PERFORMANCE,
    BALANCED,
    POWER_SAVER,
    ULTRA_LOW_POWER
};

enum class BatteryLevel {
    CRITICAL,    // < 10%
    LOW,         // 10-25%
    MEDIUM,      // 25-50%
    HIGH,        // 50-75%
    FULL,        // > 75%
    UNKNOWN
};

struct PowerSettings {
    int scanInterval;           // Bluetooth scan interval in ms
    int advertiseInterval;      // Bluetooth advertise interval in ms
    int heartbeatInterval;      // Heartbeat interval in seconds
    int connectionTimeout;      // Connection timeout in seconds
    int maxConnections;         // Maximum simultaneous connections
    bool backgroundScanning;    // Allow background scanning
    bool aggressiveScanning;    // Use aggressive scanning
    bool lowPowerMode;          // Enable low power mode
    
    PowerSettings() : scanInterval(1000), advertiseInterval(1000), heartbeatInterval(30), 
                     connectionTimeout(30), maxConnections(5), backgroundScanning(true), 
                     aggressiveScanning(false), lowPowerMode(false) {}
};

struct BatteryStats {
    int currentLevel;           // Current battery level (0-100)
    bool isCharging;            // Whether device is charging
    int timeRemaining;          // Estimated time remaining in minutes
    QDateTime lastUpdate;       // Last battery update time
    int powerSavingsMinutes;    // Minutes saved by power optimization
    
    BatteryStats() : currentLevel(100), isCharging(false), timeRemaining(-1), 
                    powerSavingsMinutes(0) {}
};

class BatteryOptimizer : public QObject
{
    Q_OBJECT

public:
    explicit BatteryOptimizer(ConfigManager* configManager, QObject* parent = nullptr);
    ~BatteryOptimizer();

    bool initialize();
    void shutdown();

    // Power mode management
    void setPowerMode(PowerMode mode);
    PowerMode getPowerMode() const;
    void setAutomaticPowerMode(bool enabled);
    bool isAutomaticPowerMode() const;

    // Battery monitoring
    BatteryLevel getBatteryLevel() const;
    BatteryStats getBatteryStats() const;
    bool isBatteryLow() const;
    bool isBatteryCritical() const;
    bool isCharging() const;

    // Power optimization
    void optimizeForBatteryLife();
    void optimizeForPerformance();
    void resetToDefaults();
    void applyEmergencyPowerSaving();

    // Service integration
    void setBluetoothService(BluetoothMeshService* service);
    void updateBluetoothSettings();

    // Configuration
    void setLowBatteryThreshold(int percentage);
    int getLowBatteryThreshold() const;
    void setCriticalBatteryThreshold(int percentage);
    int getCriticalBatteryThreshold() const;
    void setBackgroundScanningEnabled(bool enabled);
    bool isBackgroundScanningEnabled() const;

    // Statistics and monitoring
    void resetStatistics();
    int getPowerSavingsMinutes() const;
    QString getPowerModeString() const;
    QStringList getOptimizationSuggestions() const;

signals:
    void batteryLevelChanged(int level);
    void batteryLow();
    void batteryCritical();
    void chargingStateChanged(bool charging);
    void powerModeChanged(PowerMode mode);
    void powerSavingsUpdated(int minutes);

private slots:
    void updateBatteryStatus();
    void checkPowerOptimization();
    void onLowBattery();
    void onCriticalBattery();

private:
    void setupBatteryMonitoring();
    void loadPowerSettings();
    void savePowerSettings();
    void applyPowerSettings(const PowerSettings& settings);
    PowerSettings getPowerSettingsForMode(PowerMode mode) const;
    void updateAutomaticPowerMode();
    void calculatePowerSavings();
    QString getBatteryInfoFromSystem() const;
    int parseBatteryLevel(const QString& batteryInfo) const;
    bool parseChargingState(const QString& batteryInfo) const;
    void logPowerEvent(const QString& event);

    ConfigManager* m_configManager;
    BluetoothMeshService* m_bluetoothService;
    
    // Timers
    QTimer* m_batteryTimer;
    QTimer* m_optimizationTimer;
    
    // Current state
    PowerMode m_currentPowerMode;
    BatteryStats m_batteryStats;
    bool m_automaticPowerMode;
    bool m_initialized;
    
    // Configuration
    int m_lowBatteryThreshold;
    int m_criticalBatteryThreshold;
    bool m_backgroundScanningEnabled;
    
    // Power settings for different modes
    PowerSettings m_currentSettings;
    PowerSettings m_defaultSettings;
    
    // Statistics
    QDateTime m_lastOptimizationTime;
    int m_totalPowerSavings;
    
    // Constants
    static const int BATTERY_CHECK_INTERVAL = 30000; // 30 seconds
    static const int OPTIMIZATION_CHECK_INTERVAL = 60000; // 1 minute
    static const int DEFAULT_LOW_BATTERY_THRESHOLD = 25;
    static const int DEFAULT_CRITICAL_BATTERY_THRESHOLD = 10;
};

#endif // BATTERYOPTIMIZER_H 