#pragma once
#include <cassert>
#include <deque>
#include "pico/time.h"

// Single-threaded substitute. These tests do not simulate hardware spinlocks.
template<class T> class PicoQueue {
public:
    explicit PicoQueue(uint capacity) : capacity(capacity) {}
    bool isEmpty() const { return data.empty(); }
    bool isFull() const { return data.size() == capacity; }
    bool tryAdd(T *value) {
        if (isFull()) return false;
        data.push_back(*value);
        return true;
    }
    bool tryRemove(T *value) {
        if (isEmpty()) return false;
        *value = data.front();
        data.pop_front();
        return true;
    }
    void addBlocking(T *value) { assert(tryAdd(value)); }
    void removeBlocking(T *value) { assert(tryRemove(value)); }
private:
    uint capacity;
    std::deque<T> data;
};
