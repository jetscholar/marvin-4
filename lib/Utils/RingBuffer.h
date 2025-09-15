#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <Arduino.h>

template<typename T>
class RingBuffer {
public:
    RingBuffer(size_t size);
    ~RingBuffer();
    void write(T value);
    size_t read(T* data, size_t length);
    void reset();
    size_t available();

private:
    T* buffer;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
};

#endif