#include <iostream>
#include <random>
#include <deque>
#include <stdexcept>

#include "Queue.hpp"

template <typename T>
class ReferenceQueue {
    std::deque<T> data;
    int cap;
public:
    explicit ReferenceQueue(int capacity) : cap(capacity) {
        if (capacity <= 0) throw std::invalid_argument("capacity must be positive");
    }

    bool isEmpty() const { return data.empty(); }
    bool isFull()  const { return (int)data.size() == cap; }

    void enqueue(const T& x) {
        if (isFull()) throw std::overflow_error("Queue is full");
        data.push_back(x);
    }

    T dequeue() {
        if (isEmpty()) throw std::underflow_error("Queue is empty");
        T val = data.front();
        data.pop_front();
        return val;
    }

    const T& peek() const {
        if (isEmpty()) throw std::underflow_error("Queue is empty");
        return data.front();
    }

    int size() const { return (int)data.size(); }
    int capacity() const { return cap; }
};

int main() {
    std::cout << "Starting stress test..." << std::endl;

    const int CAPACITY = 30;
    const int ITERATIONS = 1000;
    const int OPS_PER_ITERATION = 50;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> opDist(0, 2);       // 0:enqueue, 1:dequeue, 2:peek+size
    std::uniform_int_distribution<int> valueDist(0, 999);

    try {
        for (int iter = 0; iter < ITERATIONS; ++iter) {
            Queue<int> testQ(CAPACITY);
            ReferenceQueue<int> refQ(CAPACITY);

            for (int op = 0; op < OPS_PER_ITERATION; ++op) {
                int action = opDist(rng);

                switch (action) {
                    case 0: { // enqueue
                        bool testFull = testQ.isFull();
                        bool refFull  = refQ.isFull();
                        if (testFull != refFull) {
                            std::cerr << "ERROR: isFull mismatch at iter=" << iter
                                      << " op=" << op << "\n";
                            return 1;
                        }

                        if (!testFull) {
                            int val = valueDist(rng);
                            testQ.enqueue(val);
                            refQ.enqueue(val);
                        } else {
                            try {
                                testQ.enqueue(0);
                                std::cerr << "ERROR: expected overflow_error at iter=" << iter
                                          << " op=" << op << "\n";
                                return 1;
                            } catch (const std::overflow_error&) {}
                        }
                        break;
                    }

                    case 1: { // dequeue
                        bool testEmpty = testQ.isEmpty();
                        bool refEmpty  = refQ.isEmpty();
                        if (testEmpty != refEmpty) {
                            std::cerr << "ERROR: isEmpty mismatch at iter=" << iter
                                      << " op=" << op << "\n";
                            return 1;
                        }

                        if (!testEmpty) {
                            int testVal = testQ.dequeue();
                            int refVal  = refQ.dequeue();
                            if (testVal != refVal) {
                                std::cerr << "ERROR: dequeue value mismatch at iter=" << iter
                                          << " op=" << op << ": test=" << testVal
                                          << " ref=" << refVal << "\n";
                                return 1;
                            }
                        } else {
                            try {
                                testQ.dequeue();
                                std::cerr << "ERROR: expected underflow_error at iter=" << iter
                                          << " op=" << op << "\n";
                                return 1;
                            } catch (const std::underflow_error&) {}
                        }
                        break;
                    }

                    case 2: { // peek + check size
                        bool testEmpty = testQ.isEmpty();
                        bool refEmpty  = refQ.isEmpty();
                        if (testEmpty != refEmpty) {
                            std::cerr << "ERROR: isEmpty mismatch (peek) at iter=" << iter
                                      << " op=" << op << "\n";
                            return 1;
                        }
                        if (!testEmpty) {
                            int testPeek = testQ.peek();
                            int refPeek  = refQ.peek();
                            if (testPeek != refPeek) {
                                std::cerr << "ERROR: peek mismatch at iter=" << iter
                                          << " op=" << op << ": test=" << testPeek
                                          << " ref=" << refPeek << "\n";
                                return 1;
                            }
                        }

                        if (testQ.size() != refQ.size()) {
                            std::cerr << "ERROR: size mismatch at iter=" << iter
                                      << " op=" << op << ": test=" << testQ.size()
                                      << " ref=" << refQ.size() << "\n";
                            return 1;
                        }
                        if (testQ.capacity() != CAPACITY) {
                            std::cerr << "ERROR: capacity mismatch at iter=" << iter << "\n";
                            return 1;
                        }
                        break;
                    }

                    default:
                        break;
                }
            }
        }

        std::cout << "All stress tests passed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }
}
