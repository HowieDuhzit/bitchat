#include "KeychainManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <sodium.h>

// Category constants
const QString KeychainManager::CHANNEL_PASSWORDS_CATEGORY = "channel_passwords";
const QString KeychainManager::IDENTITY_KEYS_CATEGORY = "identity_keys";
const QString KeychainManager::PEER_KEYS_CATEGORY = "peer_keys";
const QString KeychainManager::GENERIC_DATA_CATEGORY = "generic_data";

KeychainManager* KeychainManager::s_instance = nullptr;

KeychainManager::KeychainManager()
    : m_service("com.bitchat.keychain")
{
    // Initialize libsodium
    if (sodium_init() < 0) {
        qWarning() << "Failed to initialize libsodium";
        return;
    }
    
    // Set up secure storage directory
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_storageDir = appDataDir + "/keychain";
    
    QDir dir;
    if (!dir.mkpath(m_storageDir)) {
        qWarning() << "Failed to create keychain storage directory:" << m_storageDir;
    }
    
    // Ensure master key exists
    if (getMasterKey().isEmpty()) {
        createMasterKey();
    }
}

KeychainManager::~KeychainManager()
{
    // Clear sensitive data
    if (!m_masterKey.isEmpty()) {
        sodium_memzero(m_masterKey.data(), m_masterKey.size());
    }
}

KeychainManager* KeychainManager::shared()
{
    if (!s_instance) {
        s_instance = new KeychainManager();
    }
    return s_instance;
}

bool KeychainManager::saveChannelPassword(const QString& password, const QString& channel)
{
    QString key = QString("channel_%1").arg(channel);
    return saveToSecureStorage(password.toUtf8(), key, CHANNEL_PASSWORDS_CATEGORY);
}

QString KeychainManager::getChannelPassword(const QString& channel) const
{
    QString key = QString("channel_%1").arg(channel);
    QByteArray data = loadFromSecureStorage(key, CHANNEL_PASSWORDS_CATEGORY);
    return QString::fromUtf8(data);
}

bool KeychainManager::deleteChannelPassword(const QString& channel)
{
    QString key = QString("channel_%1").arg(channel);
    return deleteFromSecureStorage(key, CHANNEL_PASSWORDS_CATEGORY);
}

bool KeychainManager::saveIdentityKey(const QByteArray& keyData, const QString& keyName)
{
    return saveToSecureStorage(keyData, keyName, IDENTITY_KEYS_CATEGORY);
}

QByteArray KeychainManager::getIdentityKey(const QString& keyName) const
{
    return loadFromSecureStorage(keyName, IDENTITY_KEYS_CATEGORY);
}

bool KeychainManager::deleteIdentityKey(const QString& keyName)
{
    return deleteFromSecureStorage(keyName, IDENTITY_KEYS_CATEGORY);
}

bool KeychainManager::savePeerPublicKey(const QByteArray& keyData, const QString& peerID)
{
    QString key = QString("peer_%1").arg(peerID);
    return saveToSecureStorage(keyData, key, PEER_KEYS_CATEGORY);
}

QByteArray KeychainManager::getPeerPublicKey(const QString& peerID) const
{
    QString key = QString("peer_%1").arg(peerID);
    return loadFromSecureStorage(key, PEER_KEYS_CATEGORY);
}

bool KeychainManager::deletePeerPublicKey(const QString& peerID)
{
    QString key = QString("peer_%1").arg(peerID);
    return deleteFromSecureStorage(key, PEER_KEYS_CATEGORY);
}

bool KeychainManager::saveData(const QByteArray& data, const QString& key)
{
    return saveToSecureStorage(data, key, GENERIC_DATA_CATEGORY);
}

QByteArray KeychainManager::getData(const QString& key) const
{
    return loadFromSecureStorage(key, GENERIC_DATA_CATEGORY);
}

bool KeychainManager::deleteData(const QString& key)
{
    return deleteFromSecureStorage(key, GENERIC_DATA_CATEGORY);
}

bool KeychainManager::deleteAllPasswords()
{
    QDir dir(m_storageDir + "/" + CHANNEL_PASSWORDS_CATEGORY);
    if (dir.exists()) {
        return dir.removeRecursively();
    }
    return true;
}

bool KeychainManager::deleteAllKeys()
{
    bool success = true;
    
    QStringList categories = {
        IDENTITY_KEYS_CATEGORY,
        PEER_KEYS_CATEGORY,
        GENERIC_DATA_CATEGORY
    };
    
    for (const QString& category : categories) {
        QDir dir(m_storageDir + "/" + category);
        if (dir.exists()) {
            success &= dir.removeRecursively();
        }
    }
    
    return success;
}

void KeychainManager::setService(const QString& service)
{
    m_service = service;
}

QString KeychainManager::getService() const
{
    return m_service;
}

bool KeychainManager::saveToSecureStorage(const QByteArray& data, const QString& key, const QString& category)
{
    if (data.isEmpty() || key.isEmpty()) {
        return false;
    }
    
    // Create category directory
    QString categoryDir = m_storageDir + "/" + category;
    QDir dir;
    if (!dir.mkpath(categoryDir)) {
        qWarning() << "Failed to create category directory:" << categoryDir;
        return false;
    }
    
    // Encrypt the data
    QByteArray encryptedData = encryptData(data);
    if (encryptedData.isEmpty()) {
        qWarning() << "Failed to encrypt data for key:" << key;
        return false;
    }
    
    // Save to file
    QString filePath = categoryDir + "/" + key + ".enc";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    qint64 written = file.write(encryptedData);
    file.close();
    
    if (written != encryptedData.size()) {
        qWarning() << "Failed to write complete data to file:" << filePath;
        return false;
    }
    
    // Set restrictive permissions (owner read/write only)
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    
    return true;
}

QByteArray KeychainManager::loadFromSecureStorage(const QString& key, const QString& category) const
{
    if (key.isEmpty()) {
        return QByteArray();
    }
    
    QString filePath = m_storageDir + "/" + category + "/" + key + ".enc";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        // File doesn't exist or can't be opened
        return QByteArray();
    }
    
    QByteArray encryptedData = file.readAll();
    file.close();
    
    if (encryptedData.isEmpty()) {
        return QByteArray();
    }
    
    // Decrypt the data
    return decryptData(encryptedData);
}

bool KeychainManager::deleteFromSecureStorage(const QString& key, const QString& category)
{
    if (key.isEmpty()) {
        return false;
    }
    
    QString filePath = m_storageDir + "/" + category + "/" + key + ".enc";
    QFile file(filePath);
    if (!file.exists()) {
        return true; // Already deleted
    }
    
    return file.remove();
}

QByteArray KeychainManager::encryptData(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return QByteArray();
    }
    
    QByteArray masterKey = getMasterKey();
    if (masterKey.size() != crypto_secretbox_KEYBYTES) {
        qWarning() << "Invalid master key size";
        return QByteArray();
    }
    
    // Generate random nonce
    QByteArray nonce(crypto_secretbox_NONCEBYTES, 0);
    randombytes_buf(nonce.data(), nonce.size());
    
    // Encrypt the data
    QByteArray ciphertext(data.size() + crypto_secretbox_MACBYTES, 0);
    
    if (crypto_secretbox_easy(reinterpret_cast<unsigned char*>(ciphertext.data()),
                             reinterpret_cast<const unsigned char*>(data.data()),
                             data.size(),
                             reinterpret_cast<const unsigned char*>(nonce.data()),
                             reinterpret_cast<const unsigned char*>(masterKey.data())) != 0) {
        qWarning() << "Failed to encrypt data";
        return QByteArray();
    }
    
    // Prepend nonce to ciphertext
    return nonce + ciphertext;
}

QByteArray KeychainManager::decryptData(const QByteArray& encryptedData) const
{
    if (encryptedData.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
        qWarning() << "Invalid encrypted data size";
        return QByteArray();
    }
    
    QByteArray masterKey = getMasterKey();
    if (masterKey.size() != crypto_secretbox_KEYBYTES) {
        qWarning() << "Invalid master key size";
        return QByteArray();
    }
    
    // Extract nonce and ciphertext
    QByteArray nonce = encryptedData.left(crypto_secretbox_NONCEBYTES);
    QByteArray ciphertext = encryptedData.mid(crypto_secretbox_NONCEBYTES);
    
    // Decrypt the data
    QByteArray plaintext(ciphertext.size() - crypto_secretbox_MACBYTES, 0);
    
    if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char*>(plaintext.data()),
                                  reinterpret_cast<const unsigned char*>(ciphertext.data()),
                                  ciphertext.size(),
                                  reinterpret_cast<const unsigned char*>(nonce.data()),
                                  reinterpret_cast<const unsigned char*>(masterKey.data())) != 0) {
        qWarning() << "Failed to decrypt data";
        return QByteArray();
    }
    
    return plaintext;
}

QByteArray KeychainManager::getMasterKey() const
{
    if (!m_masterKey.isEmpty()) {
        return m_masterKey;
    }
    
    QString keyFilePath = m_storageDir + "/master.key";
    QFile file(keyFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    
    m_masterKey = file.readAll();
    file.close();
    
    return m_masterKey;
}

bool KeychainManager::createMasterKey()
{
    // Generate a new master key
    QByteArray newKey(crypto_secretbox_KEYBYTES, 0);
    randombytes_buf(newKey.data(), newKey.size());
    
    QString keyFilePath = m_storageDir + "/master.key";
    QFile file(keyFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create master key file:" << keyFilePath;
        return false;
    }
    
    qint64 written = file.write(newKey);
    file.close();
    
    if (written != newKey.size()) {
        qWarning() << "Failed to write complete master key";
        return false;
    }
    
    // Set restrictive permissions (owner read/write only)
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    
    m_masterKey = newKey;
    return true;
} 