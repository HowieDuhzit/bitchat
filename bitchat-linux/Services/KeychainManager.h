#pragma once

#include <QString>
#include <QByteArray>
#include <QMap>
#include <memory>

class KeychainManager
{
public:
    static KeychainManager* shared();
    
    // Channel passwords
    bool saveChannelPassword(const QString& password, const QString& channel);
    QString getChannelPassword(const QString& channel) const;
    bool deleteChannelPassword(const QString& channel);
    
    // Identity keys (for encryption)
    bool saveIdentityKey(const QByteArray& keyData, const QString& keyName);
    QByteArray getIdentityKey(const QString& keyName) const;
    bool deleteIdentityKey(const QString& keyName);
    
    // Peer public keys
    bool savePeerPublicKey(const QByteArray& keyData, const QString& peerID);
    QByteArray getPeerPublicKey(const QString& peerID) const;
    bool deletePeerPublicKey(const QString& peerID);
    
    // Generic key-value storage
    bool saveData(const QByteArray& data, const QString& key);
    QByteArray getData(const QString& key) const;
    bool deleteData(const QString& key);
    
    // Cleanup
    bool deleteAllPasswords();
    bool deleteAllKeys();
    
    // Configuration
    void setService(const QString& service);
    QString getService() const;
    
private:
    KeychainManager();
    ~KeychainManager();
    
    static KeychainManager* s_instance;
    
    // Linux-specific secure storage implementation
    bool saveToSecureStorage(const QByteArray& data, const QString& key, const QString& category);
    QByteArray loadFromSecureStorage(const QString& key, const QString& category) const;
    bool deleteFromSecureStorage(const QString& key, const QString& category);
    
    // Encryption for local storage
    QByteArray encryptData(const QByteArray& data) const;
    QByteArray decryptData(const QByteArray& encryptedData) const;
    
    // Master key management
    QByteArray getMasterKey() const;
    bool createMasterKey();
    
    QString m_service;
    QString m_storageDir;
    mutable QByteArray m_masterKey;
    
    // Categories for different types of data
    static const QString CHANNEL_PASSWORDS_CATEGORY;
    static const QString IDENTITY_KEYS_CATEGORY;
    static const QString PEER_KEYS_CATEGORY;
    static const QString GENERIC_DATA_CATEGORY;
}; 