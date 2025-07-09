#pragma once

#include <vector>
#include <string>
#include <cstdint>

class OptimizedBloomFilter {
public:
    // Create a bloom filter with optimal parameters for expected items
    static OptimizedBloomFilter adaptive(int expectedItems);
    
    // Create a bloom filter with specific parameters
    OptimizedBloomFilter(size_t size, size_t numHashFunctions);
    
    // Add an item to the filter
    void add(const std::vector<uint8_t>& data);
    
    // Check if an item might be in the filter
    bool contains(const std::vector<uint8_t>& data) const;
    
    // Clear the filter
    void clear();
    
    // Estimate the current false positive probability
    double estimateFalsePositiveProbability() const;
    
private:
    // Compute a hash value for data with a given seed
    size_t computeHash(const std::vector<uint8_t>& data, size_t seed) const;
    
    size_t m_size;               // Size of bit array
    size_t m_numHashFunctions;   // Number of hash functions
    std::vector<bool> m_bits;    // Bit array
}; 