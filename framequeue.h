#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H

#include <mutex>
#include <vector>
#include "framedata.h"

class FrameQueue {
public:
    // Constructor to set the capacity
    FrameQueue(size_t capacity);

    // Enqueue data into the queue
    void enqueue(FrameData frameData);

    // Dequeue data from the queue
    bool dequeue(FrameData& outFrame);

    // Clear the queue
    void clear();

    // Check if the queue is empty
    bool isEmpty() const;

    // Check if the queue is full
    bool isFull() const;

private:
    // Circular buffer to store FrameData
    std::vector<FrameData> buffer;

    // Queue indices and capacity
    size_t head;
    size_t tail;
    size_t capacity;
    size_t size;

    // Mutex for thread safety
    mutable std::mutex mtx;
};

#endif // FRAMEQUEUE_H
