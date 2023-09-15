#include "AtomicTimestampOffset.h"

uint64_t AtomicTimestampOffset::get(uint64_t timestamp)
{
    uint64_t out;
    uint64_t t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    mutex.lock();
    if(!set)
    {
        offset = t - timestamp;
        set = true;
    }
    out = offset + timestamp;
    mutex.unlock();
    return out;
}

void AtomicTimestampOffset::unset()
{
    mutex.lock();
    set = false;
    mutex.unlock();
}