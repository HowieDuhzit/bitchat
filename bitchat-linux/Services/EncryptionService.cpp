#include "EncryptionService.h"
#include <sodium.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

EncryptionService::EncryptionService() {
    // Initialize libsodium
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium");
    }
}

EncryptionService::~EncryptionService() {
    // Clear sensitive data
    if (!m_privateKey.empty()) {
        sodium_memzero(m_privateKey.data(), m_privateKey.size());
    }
    if (!m_signPrivateKey.empty()) {
        sodium_memzero(m_signPrivateKey.data(), m_signPrivateKey.size());
    }
}

bool EncryptionService::initialize() {
    // Generate key pairs if they don't exist
    if (m_publicKey.empty() || m_privateKey.empty()) {
        if (!generateKeyPair()) {
            return false;
        }
    }
    
    return true;
}

std::string EncryptionService::getPublicKey() const {
    return bytesToHex(m_publicKey);
}

std::string EncryptionService::getPrivateKey() const {
    return bytesToHex(m_privateKey);
}

bool EncryptionService::generateKeyPair() {
    // Generate X25519 key pair for encryption
    m_publicKey.resize(crypto_box_PUBLICKEYBYTES);
    m_privateKey.resize(crypto_box_SECRETKEYBYTES);
    
    if (crypto_box_keypair(m_publicKey.data(), m_privateKey.data()) != 0) {
        return false;
    }
    
    // Generate Ed25519 key pair for signing
    m_signPublicKey.resize(crypto_sign_PUBLICKEYBYTES);
    m_signPrivateKey.resize(crypto_sign_SECRETKEYBYTES);
    
    if (crypto_sign_keypair(m_signPublicKey.data(), m_signPrivateKey.data()) != 0) {
        return false;
    }
    
    return true;
}

bool EncryptionService::loadKeyPair(const std::string& publicKey, const std::string& privateKey) {
    try {
        m_publicKey = hexToBytes(publicKey);
        m_privateKey = hexToBytes(privateKey);
        
        if (m_publicKey.size() != crypto_box_PUBLICKEYBYTES ||
            m_privateKey.size() != crypto_box_SECRETKEYBYTES) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool EncryptionService::saveKeyPair(const std::string& filePath) const {
    if (!isInitialized()) {
        return false;
    }
    
    try {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // Write public key
        file.write(reinterpret_cast<const char*>(m_publicKey.data()), m_publicKey.size());
        
        // Write private key
        file.write(reinterpret_cast<const char*>(m_privateKey.data()), m_privateKey.size());
        
        // Write signing public key
        file.write(reinterpret_cast<const char*>(m_signPublicKey.data()), m_signPublicKey.size());
        
        // Write signing private key
        file.write(reinterpret_cast<const char*>(m_signPrivateKey.data()), m_signPrivateKey.size());
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool EncryptionService::loadKeyPair(const std::string& filePath) {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // Read public key
        m_publicKey.resize(crypto_box_PUBLICKEYBYTES);
        file.read(reinterpret_cast<char*>(m_publicKey.data()), m_publicKey.size());
        
        // Read private key
        m_privateKey.resize(crypto_box_SECRETKEYBYTES);
        file.read(reinterpret_cast<char*>(m_privateKey.data()), m_privateKey.size());
        
        // Read signing public key
        m_signPublicKey.resize(crypto_sign_PUBLICKEYBYTES);
        file.read(reinterpret_cast<char*>(m_signPublicKey.data()), m_signPublicKey.size());
        
        // Read signing private key
        m_signPrivateKey.resize(crypto_sign_SECRETKEYBYTES);
        file.read(reinterpret_cast<char*>(m_signPrivateKey.data()), m_signPrivateKey.size());
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void EncryptionService::addPeerPublicKey(const std::string& peerId, const std::string& publicKey) {
    try {
        m_peerPublicKeys[peerId] = hexToBytes(publicKey);
    } catch (const std::exception& e) {
        // Invalid key format
    }
}

void EncryptionService::removePeerPublicKey(const std::string& peerId) {
    m_peerPublicKeys.erase(peerId);
}

bool EncryptionService::hasPeerPublicKey(const std::string& peerId) const {
    return m_peerPublicKeys.find(peerId) != m_peerPublicKeys.end();
}

std::string EncryptionService::getPeerPublicKey(const std::string& peerId) const {
    auto it = m_peerPublicKeys.find(peerId);
    if (it != m_peerPublicKeys.end()) {
        return bytesToHex(it->second);
    }
    return "";
}

std::vector<uint8_t> EncryptionService::encrypt(const std::vector<uint8_t>& data, const std::string& recipientPublicKey) const {
    if (!isInitialized()) {
        return {};
    }
    
    try {
        std::vector<uint8_t> recipientPubKey = hexToBytes(recipientPublicKey);
        if (recipientPubKey.size() != crypto_box_PUBLICKEYBYTES) {
            return {};
        }
        
        // Generate random nonce
        std::vector<uint8_t> nonce = generateNonce();
        
        // Encrypt the data
        std::vector<uint8_t> ciphertext(data.size() + crypto_box_MACBYTES);
        
        if (crypto_box_easy(ciphertext.data(), data.data(), data.size(),
                           nonce.data(), recipientPubKey.data(), m_privateKey.data()) != 0) {
            return {};
        }
        
        // Prepend nonce to ciphertext
        std::vector<uint8_t> result;
        result.insert(result.end(), nonce.begin(), nonce.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());
        
        return result;
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<uint8_t> EncryptionService::decrypt(const std::vector<uint8_t>& encryptedData) const {
    if (!isInitialized() || encryptedData.size() < crypto_box_NONCEBYTES + crypto_box_MACBYTES) {
        return {};
    }
    
    try {
        // Extract nonce
        std::vector<uint8_t> nonce(encryptedData.begin(), encryptedData.begin() + crypto_box_NONCEBYTES);
        
        // Extract ciphertext
        std::vector<uint8_t> ciphertext(encryptedData.begin() + crypto_box_NONCEBYTES, encryptedData.end());
        
        // Decrypt the data
        std::vector<uint8_t> plaintext(ciphertext.size() - crypto_box_MACBYTES);
        
        // Try to decrypt with each known peer's public key
        for (const auto& peer : m_peerPublicKeys) {
            if (crypto_box_open_easy(plaintext.data(), ciphertext.data(), ciphertext.size(),
                                   nonce.data(), peer.second.data(), m_privateKey.data()) == 0) {
                return plaintext;
            }
        }
        
        return {};
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<uint8_t> EncryptionService::encryptChannel(const std::vector<uint8_t>& data, const std::string& channelKey) const {
    try {
        std::vector<uint8_t> key = hexToBytes(channelKey);
        if (key.size() != crypto_secretbox_KEYBYTES) {
            return {};
        }
        
        // Generate random nonce
        std::vector<uint8_t> nonce = generateNonce();
        
        // Encrypt the data
        std::vector<uint8_t> ciphertext(data.size() + crypto_secretbox_MACBYTES);
        
        if (crypto_secretbox_easy(ciphertext.data(), data.data(), data.size(),
                                nonce.data(), key.data()) != 0) {
            return {};
        }
        
        // Prepend nonce to ciphertext
        std::vector<uint8_t> result;
        result.insert(result.end(), nonce.begin(), nonce.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());
        
        return result;
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<uint8_t> EncryptionService::decryptChannel(const std::vector<uint8_t>& encryptedData, const std::string& channelKey) const {
    if (encryptedData.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
        return {};
    }
    
    try {
        std::vector<uint8_t> key = hexToBytes(channelKey);
        if (key.size() != crypto_secretbox_KEYBYTES) {
            return {};
        }
        
        // Extract nonce
        std::vector<uint8_t> nonce(encryptedData.begin(), encryptedData.begin() + crypto_secretbox_NONCEBYTES);
        
        // Extract ciphertext
        std::vector<uint8_t> ciphertext(encryptedData.begin() + crypto_secretbox_NONCEBYTES, encryptedData.end());
        
        // Decrypt the data
        std::vector<uint8_t> plaintext(ciphertext.size() - crypto_secretbox_MACBYTES);
        
        if (crypto_secretbox_open_easy(plaintext.data(), ciphertext.data(), ciphertext.size(),
                                     nonce.data(), key.data()) != 0) {
            return {};
        }
        
        return plaintext;
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<uint8_t> EncryptionService::sign(const std::vector<uint8_t>& data) const {
    if (!isInitialized()) {
        return {};
    }
    
    std::vector<uint8_t> signature(crypto_sign_BYTES);
    
    if (crypto_sign_detached(signature.data(), nullptr, data.data(), data.size(),
                           m_signPrivateKey.data()) != 0) {
        return {};
    }
    
    return signature;
}

bool EncryptionService::verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature, const std::string& publicKey) const {
    if (signature.size() != crypto_sign_BYTES) {
        return false;
    }
    
    try {
        std::vector<uint8_t> pubKey = hexToBytes(publicKey);
        if (pubKey.size() != crypto_sign_PUBLICKEYBYTES) {
            return false;
        }
        
        return crypto_sign_verify_detached(signature.data(), data.data(), data.size(),
                                         pubKey.data()) == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string EncryptionService::deriveChannelKey(const std::string& channelName, const std::string& password) const {
    std::string input = channelName + password;
    std::vector<uint8_t> key = deriveKey(input, "bitchat_channel");
    return bytesToHex(key);
}

std::string EncryptionService::deriveSharedSecret(const std::string& peerPublicKey) const {
    if (!isInitialized()) {
        return "";
    }
    
    try {
        std::vector<uint8_t> pubKey = hexToBytes(peerPublicKey);
        if (pubKey.size() != crypto_box_PUBLICKEYBYTES) {
            return "";
        }
        
        std::vector<uint8_t> sharedSecret(crypto_box_BEFORENMBYTES);
        
        if (crypto_box_beforenm(sharedSecret.data(), pubKey.data(), m_privateKey.data()) != 0) {
            return "";
        }
        
        return bytesToHex(sharedSecret);
    } catch (const std::exception& e) {
        return "";
    }
}

std::string EncryptionService::hash(const std::vector<uint8_t>& data) const {
    std::vector<uint8_t> hashBytes(crypto_hash_sha256_BYTES);
    crypto_hash_sha256(hashBytes.data(), data.data(), data.size());
    return bytesToHex(hashBytes);
}

std::string EncryptionService::hash(const std::string& data) const {
    std::vector<uint8_t> dataBytes(data.begin(), data.end());
    return hash(dataBytes);
}

std::vector<uint8_t> EncryptionService::generateRandomBytes(size_t length) const {
    std::vector<uint8_t> bytes(length);
    randombytes_buf(bytes.data(), length);
    return bytes;
}

std::string EncryptionService::generateRandomString(size_t length) const {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(length);
    
    std::vector<uint8_t> randomBytes = generateRandomBytes(length);
    for (size_t i = 0; i < length; ++i) {
        result += chars[randomBytes[i] % chars.length()];
    }
    
    return result;
}

std::string EncryptionService::bytesToHex(const std::vector<uint8_t>& bytes) const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<uint8_t> EncryptionService::hexToBytes(const std::string& hex) const {
    if (hex.length() % 2 != 0) {
        throw std::invalid_argument("Invalid hex string length");
    }
    
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.length() / 2);
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    
    return bytes;
}

std::string EncryptionService::bytesToBase64(const std::vector<uint8_t>& bytes) const {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    int val = 0;
    int valb = -6;
    
    for (uint8_t byte : bytes) {
        val = (val << 8) + byte;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (result.size() % 4) {
        result.push_back('=');
    }
    
    return result;
}

std::vector<uint8_t> EncryptionService::base64ToBytes(const std::string& base64) const {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> result;
    
    int val = 0;
    int valb = -8;
    
    for (char c : base64) {
        if (c == '=') break;
        
        size_t pos = chars.find(c);
        if (pos == std::string::npos) continue;
        
        val = (val << 6) + pos;
        valb += 6;
        
        if (valb >= 0) {
            result.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return result;
}

bool EncryptionService::isInitialized() const {
    return !m_publicKey.empty() && !m_privateKey.empty();
}

std::vector<uint8_t> EncryptionService::generateNonce() const {
    std::vector<uint8_t> nonce(crypto_secretbox_NONCEBYTES);
    randombytes_buf(nonce.data(), nonce.size());
    return nonce;
}

std::vector<uint8_t> EncryptionService::deriveKey(const std::string& input, const std::string& salt) const {
    std::vector<uint8_t> key(crypto_secretbox_KEYBYTES);
    std::vector<uint8_t> saltBytes(salt.begin(), salt.end());
    
    // Pad salt to required length
    if (saltBytes.size() < crypto_pwhash_SALTBYTES) {
        saltBytes.resize(crypto_pwhash_SALTBYTES, 0);
    }
    
    if (crypto_pwhash(key.data(), key.size(), input.c_str(), input.length(),
                     saltBytes.data(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
                     crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_DEFAULT) != 0) {
        // Fallback to simpler key derivation
        std::string combined = input + salt;
        std::vector<uint8_t> hashInput(combined.begin(), combined.end());
        crypto_hash_sha256(key.data(), hashInput.data(), hashInput.size());
    }
    
    return key;
} 