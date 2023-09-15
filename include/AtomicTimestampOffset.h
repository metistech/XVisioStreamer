#pragma once

#include <mutex>
#include <chrono>

class AtomicTimestampOffset
{
private:
    bool set;
    uint64_t offset;
    std::mutex mutex;

public:
    AtomicTimestampOffset() : set(false) {}
    uint64_t get(uint64_t timestamp);
    void unset();
};