#ifndef CP_DATA_STRUCTURES_OPEN_ADDRESSING_BACKEND_H
#define CP_DATA_STRUCTURES_OPEN_ADDRESSING_BACKEND_H

#pragma once

#include <memory>
#include <vector>

#include "IBackend.hpp"
#include "UniversalHash.hpp"

namespace hashmap {
    enum class ProbeStrategy {
        Linear, // idx = (base + i) % size
        Quadratic, // idx = (base + i*i) % size
        Double, // idx = (base + i*step) % size
    };

    namespace detail {
        template<typename K, typename V>
        class OpenAddressingBackend : public IBackend<K, V> {
        public:
            OpenAddressingBackend(size_t initial_size, ProbeStrategy strategy)
                : size_(std::max<size_t>(initial_size, 1)),
                  count_(0),
                  strategy_(strategy),
                  h1_(size_),
                  h2_(size_) {
                slots_.resize(size_);
            }

            void Insert(const K &key, const V &value) override {
                if (LoadFactor() >= kMaxLoadFactor) {
                    Grow(size_ * 2);
                }
                InsertInto(key, value);
            }

            std::optional<V> Search(const K &key) const override {
                const size_t base = BaseHash(key);
                const size_t step = StepHash(key);

                for (size_t i = 0; i < size_; ++i) {
                    const size_t idx = Probe(base, step, i);
                    const Node *node = slots_[idx].get();
                    if (node == nullptr) return std::nullopt;
                    if (!node->deleted && node->key == key) return node->value;
                }
                return std::nullopt;
            }

            bool Delete(const K &key) override {
                const size_t base = BaseHash(key);
                const size_t step = StepHash(key);

                for (size_t i = 0; i < size_; ++i) {
                    const size_t idx = Probe(base, step, i);
                    Node *node = slots_[idx].get();
                    if (node == nullptr) return false;
                    if (!node->deleted && node->key == key) {
                        node->deleted = true;
                        --count_;
                        return true;
                    }
                }
                return false;
            }

            [[nodiscard]] size_t Size() const override { return count_; }
            [[nodiscard]] size_t Capacity() const override { return size_; }

        private:
            struct Node {
                K key;
                V value;
                bool deleted;
            };

            static constexpr double kMaxLoadFactor = 0.7;

            [[nodiscard]] double LoadFactor() const {
                return static_cast<double>(count_ + 1) / static_cast<double>(size_);
            }

            size_t BaseHash(const K &key) const { return static_cast<size_t>(h1_(key)); }

            size_t StepHash(const K &key) const {
                if (size_ == 1) return 1;
                size_t step = static_cast<size_t>(h2_(key)) % (size_ - 1);
                return step + 1;
            }

            [[nodiscard]] size_t Probe(size_t base, size_t step, size_t i) const {
                switch (strategy_) {
                    case ProbeStrategy::Linear:
                        return (base + i) % size_;
                    case ProbeStrategy::Quadratic:
                        return (base + i * i) % size_;
                    case ProbeStrategy::Double:
                        return (base + i * step) % size_;
                }
                return (base + i) % size_;
            }

            void InsertInto(const K &key, const V &value) {
                const size_t base = BaseHash(key);
                const size_t step = StepHash(key);
                long long first_free = -1;

                for (size_t i = 0; i < size_; ++i) {
                    const size_t idx = Probe(base, step, i);
                    Node *node = slots_[idx].get();

                    if (node == nullptr) {
                        if (first_free == -1) first_free = static_cast<long long>(idx);
                        break;
                    }
                    if (node->deleted) {
                        if (first_free == -1) first_free = static_cast<long long>(idx);
                        continue;
                    }
                    if (node->key == key) {
                        node->value = value;
                        return;
                    }
                }

                if (first_free == -1) {
                    Grow(size_ * 2);
                    InsertInto(key, value);
                    return;
                }

                slots_[static_cast<size_t>(first_free)] = std::make_unique<Node>(Node{key, value, false});
                ++count_;
            }

            void Grow(size_t new_size) {
                std::vector<std::unique_ptr<Node> > old_slots = std::move(slots_);
                size_ = new_size;
                slots_.clear();
                slots_.resize(size_);
                count_ = 0;
                h1_.Rehash(size_);
                h2_.Rehash(size_);

                for (auto &node: old_slots) {
                    if (node && !node->deleted) {
                        InsertInto(node->key, node->value);
                    }
                }
            }

            std::vector<std::unique_ptr<Node> > slots_;
            size_t size_;
            size_t count_;
            ProbeStrategy strategy_;
            KeyHasher<K> h1_;
            KeyHasher<K> h2_;
        };
    }
}

#endif //CP_DATA_STRUCTURES_OPEN_ADDRESSING_BACKEND_H
