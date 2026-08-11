#ifndef CP_DATA_STRUCTURES_HASHMAP_HPP
#define CP_DATA_STRUCTURES_HASHMAP_HPP

#pragma once
#include <memory>
#include <stdexcept>

#include "ChainingBackend.hpp"
#include "OpenAddressingBackend.hpp"

namespace hashmap {

enum class Strategy {
    Chaining,
    LinearProbing,
    QuadraticProbing,
    DoubleHashing,
};

template <typename K, typename V>
class HashMap {
public:
    explicit HashMap(Strategy strategy, size_t initial_capacity = 16)
        : backend_(MakeBackend(strategy, initial_capacity)) {}

    void Insert(const K& key, const V& value) { backend_->Insert(key, value); }
    [[nodiscard]] std::optional<V> Search(const K& key) const { return backend_->Search(key); }
    [[nodiscard]] bool Delete(const K& key) { return backend_->Delete(key); }

    [[nodiscard]] size_t Size() const { return backend_->Size(); }
    [[nodiscard]] size_t Capacity() const { return backend_->Capacity(); }
    [[nodiscard]] double LoadFactor() const {
        return static_cast<double>(Size()) / static_cast<double>(Capacity());
    }

private:
    [[nodiscard]] static std::unique_ptr<detail::IBackend<K, V>> MakeBackend(Strategy strategy, size_t initial_capacity) {
        switch (strategy) {
            case Strategy::Chaining:
                return std::make_unique<detail::ChainingBackend<K, V>>(initial_capacity);
            case Strategy::LinearProbing:
                return std::make_unique<detail::OpenAddressingBackend<K, V>>(initial_capacity, ProbeStrategy::Linear);
            case Strategy::QuadraticProbing:
                return std::make_unique<detail::OpenAddressingBackend<K, V>>(initial_capacity, ProbeStrategy::Quadratic);
            case Strategy::DoubleHashing:
                return std::make_unique<detail::OpenAddressingBackend<K, V>>(initial_capacity, ProbeStrategy::Double);
        }
        throw std::invalid_argument("unknown strategy");
    }

    std::unique_ptr<detail::IBackend<K, V>> backend_;
};

}

#endif //CP_DATA_STRUCTURES_HASHMAP_HPP


