#ifndef CP_DATA_STRUCTURES_STACK_HPP
#define CP_DATA_STRUCTURES_STACK_HPP

#include <stdexcept>
#include <vector>

template <typename T>
class Stack {
    std::vector<T> S;
    int top;
    int n;

    public:
    explicit Stack(int capacity) {
        S = std::vector<T>(capacity + 1);
        top = 0;
        n = capacity;
    }

    [[nodiscard]] bool isEmpty() const {
        return top == 0;
    }

    [[nodiscard]] bool isFull() const {
        return top == n;
    }

    void push (const T& x) {
        if (isFull()) {
          throw std::overflow_error("stack overflow");
        }

        top = top + 1;
        S[top] = x;
    }

    T pop() {
        if (isEmpty()) {
            throw std::underflow_error("stack underflow");
        }

        top = top - 1;

        return S[top+1];
    }

    T& peak() {
        if (isEmpty()) {
            throw std::underflow_error("stack underflow");
        }

        return S[top];
    }

    [[nodiscard]] int size() const {
        return top;
    }
};

#endif //CP_DATA_STRUCTURES_STACK_HPP

