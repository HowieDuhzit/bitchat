#pragma once

#include <vector>
#include <cstdint>

class MessagePadding {
public:
    static const std::vector<int> BLOCK_SIZES;
    
    static std::vector<uint8_t> pad(const std::vector<uint8_t>& data, int targetSize);
    static std::vector<uint8_t> unpad(const std::vector<uint8_t>& data);
    static int optimalBlockSize(int dataSize);
}; 