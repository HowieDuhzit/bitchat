#include "CompressionUtil.h"
#include <zlib.h>
#include <stdexcept>
#include <QDebug>

std::vector<uint8_t> CompressionUtil::compress(const std::vector<uint8_t>& data)
{
    if (data.empty()) {
        return {};
    }
    
    // Don't compress small data
    if (!shouldCompress(data)) {
        return data;
    }
    
    uLongf compressedSize = compressBound(data.size());
    std::vector<uint8_t> compressed(compressedSize);
    
    int result = compress2(compressed.data(), &compressedSize,
                          data.data(), data.size(), COMPRESSION_LEVEL);
    
    if (result != Z_OK) {
        qWarning() << "Compression failed with error:" << result;
        return data; // Return original data if compression fails
    }
    
    compressed.resize(compressedSize);
    
    // Only return compressed data if it's actually smaller
    if (compressed.size() < data.size()) {
        return compressed;
    } else {
        return data; // Return original if compression doesn't help
    }
}

std::vector<uint8_t> CompressionUtil::decompress(const std::vector<uint8_t>& compressedData)
{
    if (compressedData.empty()) {
        return {};
    }
    
    // Start with a reasonable buffer size
    uLongf decompressedSize = compressedData.size() * 4;
    std::vector<uint8_t> decompressed;
    
    int result;
    int attempts = 0;
    const int maxAttempts = 5;
    
    do {
        decompressed.resize(decompressedSize);
        result = uncompress(decompressed.data(), &decompressedSize,
                           compressedData.data(), compressedData.size());
        
        if (result == Z_BUF_ERROR) {
            // Buffer too small, try again with larger buffer
            decompressedSize *= 2;
            attempts++;
        } else if (result == Z_OK) {
            decompressed.resize(decompressedSize);
            break;
        } else {
            qWarning() << "Decompression failed with error:" << result;
            return compressedData; // Return original data if decompression fails
        }
    } while (attempts < maxAttempts);
    
    if (result != Z_OK) {
        qWarning() << "Decompression failed after" << maxAttempts << "attempts";
        return compressedData; // Return original data
    }
    
    return decompressed;
}

bool CompressionUtil::shouldCompress(const std::vector<uint8_t>& data)
{
    return data.size() > MIN_COMPRESSION_SIZE;
}

double CompressionUtil::getCompressionRatio(const std::vector<uint8_t>& original, const std::vector<uint8_t>& compressed)
{
    if (original.empty() || compressed.empty()) {
        return 1.0;
    }
    
    return static_cast<double>(compressed.size()) / static_cast<double>(original.size());
} 