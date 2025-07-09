#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

class EncryptionService {
public:
    EncryptionService();
    ~EncryptionService();
    
    // Initialization
    bool initialize();
    
    // Key management
    std::string getPublicKey() const;
    std::string getPrivateKey() const;
    bool generateKeyPair();
    bool loadKeyPair(const std::string& publicKey, const std::string& privateKey);
    bool saveKeyPair(const std::string& filePath) const;
    bool loadKeyPair(const std::string& filePath);
    
    // Peer key management
    void addPeerPublicKey(const std::string& peerId, const std::string& publicKey);
    void removePeerPublicKey(const std::string& peerId);
    bool hasPeerPublicKey(const std::string& peerId) const;
    std::string getPeerPublicKey(const std::string& peerId) const;
    
    // Encryption/Decryption
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data, const std::string& recipientPublicKey) const;
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encryptedData) const;
    
    // Symmetric encryption for channels
    std::vector<uint8_t> encryptChannel(const std::vector<uint8_t>& data, const std::string& channelKey) const;
    std::vector<uint8_t> decryptChannel(const std::vector<uint8_t>& encryptedData, const std::string& channelKey) const;
    
    // Digital signatures
    std::vector<uint8_t> sign(const std::vector<uint8_t>& data) const;
    bool verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature, const std::string& publicKey) const;
    
    // Key derivation
    std::string deriveChannelKey(const std::string& channelName, const std::string& password = "") const;
    std::string deriveSharedSecret(const std::string& peerPublicKey) const;
    
    // Hashing
    std::string hash(const std::vector<uint8_t>& data) const;
    std::string hash(const std::string& data) const;
    
    // Random generation
    std::vector<uint8_t> generateRandomBytes(size_t length) const;
    std::string generateRandomString(size_t length) const;
    
    // Utility functions
    std::string bytesToHex(const std::vector<uint8_t>& bytes) const;
    std::vector<uint8_t> hexToBytes(const std::string& hex) const;
    std::string bytesToBase64(const std::vector<uint8_t>& bytes) const;
    std::vector<uint8_t> base64ToBytes(const std::string& base64) const;
    
private:
    // Key storage
    std::vector<uint8_t> m_publicKey;
    std::vector<uint8_t> m_privateKey;
    std::vector<uint8_t> m_signPublicKey;
    std::vector<uint8_t> m_signPrivateKey;
    std::map<std::string, std::vector<uint8_t>> m_peerPublicKeys;
    std::map<std::string, std::string> m_channelKeys;
    
    // Constants
    static const size_t PUBLIC_KEY_SIZE = 32;
    static const size_t PRIVATE_KEY_SIZE = 32;
    static const size_t SIGNATURE_SIZE = 64;
    static const size_t NONCE_SIZE = 24;
    static const size_t SYMMETRIC_KEY_SIZE = 32;
    static const size_t HASH_SIZE = 32;
    
    // Helper methods
    bool isInitialized() const;
    std::vector<uint8_t> generateNonce() const;
    std::vector<uint8_t> deriveKey(const std::string& input, const std::string& salt = "") const;
}; 