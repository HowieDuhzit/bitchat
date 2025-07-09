#pragma once

#include <vector>
#include <cstdint>

class CompressionUtil
{
public:
    // Compress data using zlib
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    
    // Decompress data using zlib
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData);
    
    // Check if data should be compressed (based on size threshold)
    static bool shouldCompress(const std::vector<uint8_t>& data);
    
    // Get compression ratio
    static double getCompressionRatio(const std::vector<uint8_t>& original, const std::vector<uint8_t>& compressed);
    
private:
    static const size_t MIN_COMPRESSION_SIZE = 64; // Only compress if data is larger than 64 bytes
    static const int COMPRESSION_LEVEL = 6; // Default compression level (1-9)
}; 