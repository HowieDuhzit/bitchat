#include "MessagePadding.h"
#include <random>

// Define block sizes
const std::vector<int> MessagePadding::BLOCK_SIZES = {64, 128, 256, 512, 1024, 2048, 4096};

std::vector<uint8_t> MessagePadding::pad(const std::vector<uint8_t>& data, int targetSize) {
    if (data.size() >= targetSize) return data;
    
    std::vector<uint8_t> padded = data;
    
    // Add padding bytes
    int paddingSize = targetSize - data.size();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < paddingSize - 1; ++i) {
        padded.push_back(dis(gen));
    }
    
    // Last byte indicates padding size
    padded.push_back(paddingSize);
    
    return padded;
}

std::vector<uint8_t> MessagePadding::unpad(const std::vector<uint8_t>& data) {
    if (data.empty()) return data;
    
    uint8_t paddingSize = data.back();
    if (paddingSize == 0 || paddingSize > data.size()) {
        return data;  // Invalid padding
    }
    
    std::vector<uint8_t> unpadded(data.begin(), data.end() - paddingSize);
    return unpadded;
}

int MessagePadding::optimalBlockSize(int dataSize) {
    for (int blockSize : BLOCK_SIZES) {
        if (dataSize <= blockSize) {
            return blockSize;
        }
    }
    return BLOCK_SIZES.back();  // Use largest block size
} 