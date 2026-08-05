#ifndef CP_DATA_STRUCTURES_QUEUE_HPP
#define CP_DATA_STRUCTURES_QUEUE_HPP

#include <stdexcept>
#include <vector>

template <typename T>
class Queue {
    std::vector<T> Q;
    int head  = 0;
    int tail  = 0;
    int count = 0;

public:
    explicit Queue(int capacity) {
        if (capacity <= 0) {
            throw std::invalid_argument("capacity must be positive");
        }

        Q = std::vector<T>(capacity);
    }

    void enqueue(const T& x) {
        if (isFull()) {
            throw std::overflow_error("Queue is full");
        }

        Q[tail] = x;
        tail = (tail + 1) % Q.size();
        count++;
    }

    T dequeue() {
        if (isEmpty()) {
            throw std::underflow_error("Queue is empty");
        }

        T oldHead = Q[head];
        head = (head + 1) % Q.size();
        count--;

        return oldHead;
    }

    const T& peek() const {
        if (isEmpty()) {
            throw std::underflow_error("Queue is empty");
        }

        return Q[head];
    }

    [[nodiscard]] bool isEmpty() const { return count == 0; }
    [[nodiscard]] bool isFull()  const { return count == Q.size(); }
    [[nodiscard]] int size()     const { return count; }
    [[nodiscard]] int capacity() const { return Q.size(); }
};

#endif //CP_DATA_STRUCTURES_QUEUE_HPP
