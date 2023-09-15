#pragma once

#include <mutex>
#include <queue>
#include <vector>
#include <functional>

template <typename T>
class AtomicQueue
{
private:
    std::mutex m;
    std::priority_queue<T, std::vector<T>, std::greater<T>> q;

public:
    bool try_push(T to_push)
    {
        if(m.try_lock()){
            q.push(to_push);
            m.unlock();
            return true;
        } else {
            return false;
        }
    }
    void push(T to_push)
    {
        m.lock();
        q.push(to_push);
        m.unlock();
    }
    void pop()
    {
        m.lock();
        q.pop();
        m.unlock();
    }
    T front()
    {
        m.lock();
        T out = q.top();
        m.unlock();
        return out;
    }
    T getAndPop()
    {
        m.lock();
        T out = q.top();
        q.pop();
        m.unlock();
        return out;
    }
    bool empty()
    {
        m.lock();
        bool out = q.empty();
        m.unlock();
        return out;
    }
    void clear()
    {
        m.lock();
        std::priority_queue<T, std::vector<T>, std::greater<T>>().swap(q);
        m.unlock();
    }

    int size()
    {
        m.lock();
        int out = q.size();
        m.unlock();
        return out;
    }
};