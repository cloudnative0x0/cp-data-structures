#ifndef CP_DATA_STRUCTURES_CHAINING_HPP
#define CP_DATA_STRUCTURES_CHAINING_HPP

#pragma once

#include <algorithm>
#include <list>
#include <utility>
#include <vector>

#include "IBackend.hpp"
#include "UniversalHash.hpp"


namespace hashmap::detail {
    template<typename K, typename V>
    class ChainingBackend : public IBackend<K, V> {
    public:
        explicit ChainingBackend(size_t initial_size, double max_load_factor = 1.0)
            : buckets_(std::max<size_t>(initial_size, 1)),
              hasher_(std::max<size_t>(initial_size, 1)),
              count_(0),
              max_load_factor_(max_load_factor) {
        }

        void Insert(const K &key, const V &value) override {
            if (LoadFactor() >= max_load_factor_) {
                Grow(buckets_.size() * 2);
            }
            auto &bucket = buckets_[BucketIndex(key)];
            for (auto &kv: bucket) {
                if (kv.first == key) {
                    kv.second = value;
                    return;
                }
            }
            bucket.emplace_back(key, value);
            ++count_;
        }

        std::optional<V> Search(const K &key) const override {
            const auto &bucket = buckets_[BucketIndex(key)];
            for (const auto &kv: bucket) {
                if (kv.first == key) return kv.second;
            }
            return std::nullopt;
        }

        bool Delete(const K &key) override {
            auto &bucket = buckets_[BucketIndex(key)];
            for (auto it = bucket.begin(); it != bucket.end(); ++it) {
                if (it->first == key) {
                    bucket.erase(it);
                    --count_;
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] size_t Size() const override { return count_; }
        [[nodiscard]] size_t Capacity() const override { return buckets_.size(); }

    private:
        size_t BucketIndex(const K &key) const { return static_cast<size_t>(hasher_(key)); }

        [[nodiscard]] double LoadFactor() const {
            return static_cast<double>(count_ + 1) / static_cast<double>(buckets_.size());
        }

        void Grow(size_t new_size) {
            std::vector<std::list<std::pair<K, V> > > new_buckets(new_size);
            hasher_.Rehash(new_size);
            for (auto &bucket: buckets_) {
                for (auto &kv: bucket) {
                    new_buckets[static_cast<size_t>(hasher_(kv.first))].push_back(std::move(kv));
                }
            }
            buckets_ = std::move(new_buckets);
        }

        std::vector<std::list<std::pair<K, V> > > buckets_;
        KeyHasher<K> hasher_;
        size_t count_;
        double max_load_factor_;
    };
}

#endif //CP_DATA_STRUCTURES_CHAINING_HPP
