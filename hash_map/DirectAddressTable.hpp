#ifndef CP_DATA_STRUCTURES_DIRECT_ADDRESS_TABLE_HPP
#define CP_DATA_STRUCTURES_DIRECT_ADDRESS_TABLE_HPP

#pragma once

#include <optional>
#include <stdexcept>
#include <vector>

namespace hashmap {
    template <typename V>
    class DirectAddressTable {
    public:
        explicit DirectAddressTable(size_t universe_size)
            : slots_(universe_size), count_(0) {}

        void Insert(size_t key, const V& value) {
            CheckRange(key);
            if (!slots_[key].has_value()) {
                ++count_;
            }
            slots_[key] = value;
        }

        std::optional<V> Search(size_t key) const {
            CheckRange(key);
            return slots_[key];
        }

        bool Delete(size_t key) {
            CheckRange(key);
            if (!slots_[key].has_value()) {
                return false;
            }
            slots_[key].reset();
            --count_;
            return true;
        }

        size_t Size() const { return count_; }
        size_t UniverseSize() const { return slots_.size(); }

    private:
        void CheckRange(size_t key) const {
            if (key >= slots_.size()) {
                throw std::out_of_range("key is outside of the table universe");
            }
        }

        std::vector<std::optional<V>> slots_;
        size_t count_;
    };
}

#endif //CP_DATA_STRUCTURES_DIRECT_ADDRESS_TABLE_HPP
