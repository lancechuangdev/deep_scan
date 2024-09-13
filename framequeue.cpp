#include "framequeue.h"

// Constructor to initialize the queue
FrameQueue::FrameQueue(size_t cap) : capacity(cap), head(0), tail(0), size(0) {
    buffer.resize(capacity);
}

// Enqueue operation
void FrameQueue::enqueue(FrameData frameData) {
    std::lock_guard<std::mutex> lock(mtx);  // Lock for thread safety

    // Check if the queue is full, discard the oldest element if necessary
    if (size == capacity) {
        head = (head + 1) % capacity;
        --size;  // Decrement size to maintain the correct size count
    }

    // Store the data in the queue
    buffer[tail] = frameData;
    tail = (tail + 1) % capacity;
    ++size;
}

// Dequeue operation
bool FrameQueue::dequeue(FrameData& frameData) {
    std::lock_guard<std::mutex> lock(mtx);  // Lock for thread safety

    if (size == 0) {
        return false;  // Queue is empty
    }

    frameData = buffer[head];
    head = (head + 1) % capacity;
    --size;
    return true;
}

// Check if the queue is empty
bool FrameQueue::isEmpty() const {
    std::lock_guard<std::mutex> lock(mtx);  // Lock for thread safety
    return size == 0;
}

// Check if the queue is full
bool FrameQueue::isFull() const {
    std::lock_guard<std::mutex> lock(mtx);  // Lock for thread safety
    return size == capacity;
}