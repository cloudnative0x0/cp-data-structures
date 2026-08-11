#ifndef CP_DATA_STRUCTURES_IBACKEND_HPP
#define CP_DATA_STRUCTURES_IBACKEND_HPP

#pragma once

#include <optional>


namespace hashmap::detail {
    template <typename K, typename V>
    class IBackend {
    public:
        virtual ~IBackend() = default;

        virtual void Insert(const K& key, const V& value) = 0;
        virtual std::optional<V> Search(const K& key) const = 0;
        virtual bool Delete(const K& key) = 0;

        [[nodiscard]] virtual size_t Size() const = 0;
        [[nodiscard]] virtual size_t Capacity() const = 0;
    };
}

#endif //CP_DATA_STRUCTURES_IBACKEND_HPP
