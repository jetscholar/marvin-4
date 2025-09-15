#include "RingBuffer.h"

template<typename T>
RingBuffer<T>::RingBuffer(size_t size) : size(size), head(0), tail(0), count(0) {
    buffer = new T[size];
}

template<typename T>
RingBuffer<T>::~RingBuffer() {
    delete[] buffer;
}

template<typename T>
void RingBuffer<T>::write(T value) {
    if (count < size) {
        buffer[head] = value;
        head = (head + 1) % size;
        count++;
    }
}

template<typename T>
size_t RingBuffer<T>::read(T* data, size_t length) {
    size_t items = min(length, count);
    for (size_t i = 0; i < items; i++) {
        data[i] = buffer[tail];
        tail = (tail + 1) % size;
        count--;
    }
    return items;
}

template<typename T>
void RingBuffer<T>::reset() {
    head = 0;
    tail = 0;
    count = 0;
}

template<typename T>
size_t RingBuffer<T>::available() {
    return count;
}

// Explicit instantiation for int16_t
template class RingBuffer<int16_t>;