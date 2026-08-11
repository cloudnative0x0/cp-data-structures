#ifndef CP_DATA_STRUCTURES_UNIVERSAL_HASH_HPP
#define CP_DATA_STRUCTURES_UNIVERSAL_HASH_HPP

#pragma once

#include <cstdint>
#include <functional>
#include <random>
#include <stdexcept>

namespace hashmap {
    class UniversalHash {
    public:
        explicit UniversalHash(uint64_t table_size, uint64_t prime = kDefaultPrime)
            : table_size_(table_size), prime_(prime) {
            if (table_size_ == 0) {
                throw std::invalid_argument("table_size must be > 0");
            }
            Reseed();
        }

        uint64_t operator()(uint64_t x) const {
            uint64_t xm = x % prime_;
            unsigned __int128 val = static_cast<unsigned __int128>(a_) * xm + b_;
            auto h = static_cast<uint64_t>(val % prime_);
            return h % table_size_;
        }

        void Rehash(uint64_t new_table_size) {
            if (new_table_size == 0) {
                throw std::invalid_argument("new_table_size must be > 0");
            }
            table_size_ = new_table_size;
            Reseed();
        }

        [[nodiscard]] uint64_t TableSize() const { return table_size_; }

    private:
        static constexpr uint64_t kDefaultPrime = (1ull << 61) - 1;

        void Reseed() {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint64_t> dist_a(1, prime_ - 1);
            std::uniform_int_distribution<uint64_t> dist_b(0, prime_ - 1);
            a_ = dist_a(gen);
            b_ = dist_b(gen);
        }

        uint64_t table_size_;
        uint64_t prime_;
        uint64_t a_ = 1;
        uint64_t b_ = 0;
    };

    template<typename K>
    class KeyHasher {
    public:
        explicit KeyHasher(uint64_t table_size) : uh_(table_size) {
        }

        uint64_t operator()(const K &key) const {
            auto x = static_cast<uint64_t>(std::hash<K>{}(key));
            return uh_(x);
        }

        void Rehash(uint64_t new_table_size) { uh_.Rehash(new_table_size); }
        [[nodiscard]] uint64_t TableSize() const { return uh_.TableSize(); }

    private:
        UniversalHash uh_;
    };
}

#endif //CP_DATA_STRUCTURES_UNIVERSAL_HASH_HPP
