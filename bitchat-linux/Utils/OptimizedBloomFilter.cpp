#include "OptimizedBloomFilter.h"
#include <cmath>
#include <random>
#include <QDebug>

OptimizedBloomFilter OptimizedBloomFilter::adaptive(int expectedItems) {
    // Calculate optimal size and hash functions for given expected items
    // with false positive probability of 0.01 (1%)
    const double falsePositiveProbability = 0.01;
    
    // Calculate optimal filter size (m) using formula: m = -n*ln(p)/(ln(2)^2)
    // where n is the number of expected items and p is the false positive probability
    size_t optimalSize = static_cast<size_t>(-expectedItems * std::log(falsePositiveProbability) / (std::log(2) * std::log(2)));
    
    // Calculate optimal number of hash functions (k) using formula: k = (m/n)*ln(2)
    size_t optimalHashFunctions = static_cast<size_t>(std::max(1.0, (optimalSize / static_cast<double>(expectedItems)) * std::log(2)));
    
    // Create bloom filter with calculated parameters
    return OptimizedBloomFilter(optimalSize, optimalHashFunctions);
}

OptimizedBloomFilter::OptimizedBloomFilter(size_t size, size_t numHashFunctions)
    : m_size(size)
    , m_numHashFunctions(numHashFunctions)
    , m_bits(size, false)
{
    qDebug() << "Created bloom filter with size" << m_size << "and" << m_numHashFunctions << "hash functions";
}

void OptimizedBloomFilter::add(const std::vector<uint8_t>& data) {
    if (data.empty()) return;
    
    for (size_t i = 0; i < m_numHashFunctions; ++i) {
        size_t hash = computeHash(data, i);
        m_bits[hash % m_size] = true;
    }
}

bool OptimizedBloomFilter::contains(const std::vector<uint8_t>& data) const {
    if (data.empty()) return false;
    
    for (size_t i = 0; i < m_numHashFunctions; ++i) {
        size_t hash = computeHash(data, i);
        if (!m_bits[hash % m_size]) {
            return false;
        }
    }
    
    return true;
}

void OptimizedBloomFilter::clear() {
    std::fill(m_bits.begin(), m_bits.end(), false);
}

double OptimizedBloomFilter::estimateFalsePositiveProbability() const {
    // Count number of bits set
    size_t setBits = 0;
    for (bool bit : m_bits) {
        if (bit) setBits++;
    }
    
    // Calculate probability that a bit is still 0
    double probBitIsZero = 1.0 - static_cast<double>(setBits) / m_size;
    
    // Calculate false positive probability
    return std::pow(1.0 - probBitIsZero, m_numHashFunctions);
}

size_t OptimizedBloomFilter::computeHash(const std::vector<uint8_t>& data, size_t seed) const {
    // Simple FNV-1a hash with seed
    const uint64_t FNV_PRIME = 1099511628211ULL;
    const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    
    uint64_t hash = FNV_OFFSET_BASIS ^ seed;
    
    for (uint8_t byte : data) {
        hash ^= byte;
        hash *= FNV_PRIME;
    }
    
    return hash;
} 