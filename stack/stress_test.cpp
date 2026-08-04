#include <cassert>
#include <iostream>
#include <random>
#include <stack>
#include <vector>
#include "Stack.hpp"

int main() {
    std::cout << "Starting..." << std::endl;
    std::mt19937 rng(42);

    const int iterations = 100000;
    const int opsPerIteration = 50;
    const int capacity = 30;

    for (int iter = 0; iter < iterations; ++iter) {
        Stack<int> mine(capacity);
        std::stack<int> ref;

        for (int op = 0; op < opsPerIteration; ++op) {
            int type = rng() % 4;

            if (type == 0 && !mine.isFull()) {
                int val = static_cast<int>(rng() % 1000);
                mine.push(val);
                ref.push(val);
            } else if (type == 1 && !ref.empty()) {
                int expected = ref.top();
                ref.pop();
                int got = mine.pop();
                if (got != expected) {
                    std::cerr << "MISMATCH on pop at iter " << iter
                              << ": expected " << expected
                              << ", got " << got << "\n";
                    return 1;
                }
            } else if (type == 2 && !ref.empty()) {
                if (mine.peak() != ref.top()) {
                    std::cerr << "MISMATCH on peak at iter " << iter << "\n";
                    return 1;
                }
            } else {
                if (mine.isEmpty() != ref.empty()) {
                    std::cerr << "MISMATCH on isEmpty at iter " << iter << "\n";
                    return 1;
                }
                if (static_cast<size_t>(mine.size()) != ref.size()) {
                    std::cerr << "MISMATCH on size at iter " << iter << "\n";
                    return 1;
                }
            }
        }
    }

    {
        Stack<int> s(2);
        bool threw = false;
        try {
            s.pop();
        } catch (const std::underflow_error&) {
            threw = true;
        }
        assert(threw && "pop on empty stack must throw underflow_error");
    }
    {
        Stack<int> s(2);
        s.push(1);
        s.push(2);
        bool threw = false;
        try {
            s.push(3);
        } catch (const std::overflow_error&) {
            threw = true;
        }
        assert(threw && "push on full stack must throw overflow_error");
    }

    std::cout << "All stack stress tests passed!\n";
    return 0;
}
