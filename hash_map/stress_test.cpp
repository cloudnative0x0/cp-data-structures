#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "DirectAddressTable.hpp"
#include "HashMap.hpp"
#include "UniversalHash.hpp"

using namespace hashmap;
using Clock = std::chrono::steady_clock;

namespace {

int g_failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "  [FAIL] " << message << "\n";
        ++g_failures;
    }
}

std::string RandomString(std::mt19937_64& rng, size_t max_len = 12) {
    static const char kAlphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<size_t> len_dist(1, max_len);
    std::uniform_int_distribution<size_t> ch_dist(0, sizeof(kAlphabet) - 2);
    size_t len = len_dist(rng);
    std::string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        s.push_back(kAlphabet[ch_dist(rng)]);
    }
    return s;
}

std::string DebugKey(int k);
std::string DebugKey(const std::string& k);

const char* StrategyName(Strategy s) {
    switch (s) {
        case Strategy::Chaining: return "Chaining";
        case Strategy::LinearProbing: return "LinearProbing";
        case Strategy::QuadraticProbing: return "QuadraticProbing";
        case Strategy::DoubleHashing: return "DoubleHashing";
    }
    return "?";
}

template <typename K>
void FuzzAgainstOracle(Strategy strategy, size_t operations, size_t key_space,
                        std::mt19937_64& rng, const std::function<K(std::mt19937_64&)>& gen_key) {
    HashMap<K, int> map(strategy, 8);
    std::unordered_map<K, int> oracle;

    std::uniform_int_distribution<int> op_dist(0, 99);
    std::uniform_int_distribution<int> value_dist(-1000000, 1000000);
    std::vector<K> known_keys;
    known_keys.reserve(key_space);

    for (size_t op = 0; op < operations; ++op) {
        int roll = op_dist(rng);
        K key = gen_key(rng);

        if (roll < 55) {
            // Insert (55%)
            int value = value_dist(rng);
            map.Insert(key, value);
            oracle[key] = value;
        } else if (roll < 85) {
            // Search (30%)
            auto got = map.Search(key);
            auto it = oracle.find(key);
            if (it == oracle.end()) {
                Check(!got.has_value(), "Search should miss for a key never inserted: " + DebugKey(key));
            } else {
                Check(got.has_value() && got.value() == it->second,
                      "Search value mismatch for key " + DebugKey(key));
            }
        } else {
            // Delete (15%)
            bool deleted = map.Delete(key);
            bool existed = oracle.erase(key) > 0;
            Check(deleted == existed, "Delete result mismatch for key " + DebugKey(key));
        }

        if (op % 4000 == 3999) {
            Check(map.Size() == oracle.size(), "Size mismatch mid-run");
        }
    }

    Check(map.Size() == oracle.size(), "Final size mismatch");
    for (const auto& [k, v] : oracle) {
        auto got = map.Search(k);
        Check(got.has_value() && got.value() == v, "Final content mismatch for key " + DebugKey(k));
    }

    std::cout << "  [" << StrategyName(strategy) << "] fuzz OK: " << operations
              << " ops, final size=" << map.Size() << ", capacity=" << map.Capacity()
              << ", load_factor=" << std::fixed << std::setprecision(3) << map.LoadFactor() << "\n";
}

std::string DebugKey(int k) { return std::to_string(k); }
std::string DebugKey(const std::string& k) { return "\"" + k + "\""; }

void GrowthStressTest(Strategy strategy, size_t n) {
    HashMap<int, int> map(strategy, 4);
    for (int i = 0; i < static_cast<int>(n); ++i) {
        map.Insert(i, i * 31 + 7);
    }
    Check(map.Size() == n, "Growth: final size mismatch");
    Check(map.LoadFactor() <= 1.0, "Growth: load factor exceeded 1.0");

    std::mt19937_64 rng(123);
    std::uniform_int_distribution<int> idx_dist(0, static_cast<int>(n) - 1);
    for (int i = 0; i < 2000; ++i) {
        int k = idx_dist(rng);
        auto v = map.Search(k);
        Check(v.has_value() && v.value() == k * 31 + 7, "Growth: lost key " + std::to_string(k) + " after rehash");
    }

    std::cout << "  [" << StrategyName(strategy) << "] growth OK: n=" << n
              << ", capacity=" << map.Capacity() << ", load_factor=" << std::fixed
              << std::setprecision(3) << map.LoadFactor() << "\n";
}

void TimingBenchmark(Strategy strategy, size_t n) {
    std::mt19937_64 rng(777);
    std::vector<int> keys(n);
    for (size_t i = 0; i < n; ++i) keys[i] = static_cast<int>(i);
    std::shuffle(keys.begin(), keys.end(), rng);

    HashMap<int, int> map(strategy, 16);

    auto t0 = Clock::now();
    for (int k : keys) {
        map.Insert(k, k);
    }
    auto t1 = Clock::now();

    std::shuffle(keys.begin(), keys.end(), rng);
    volatile long sink = 0;
    for (int k : keys) {
        sink += map.Search(k).value_or(-1);
    }
    auto t2 = Clock::now();

    std::shuffle(keys.begin(), keys.end(), rng);
    for (int k : keys) {
        std::ignore = map.Delete(k);
    }
    auto t3 = Clock::now();

    auto ms = [](Clock::time_point a, Clock::time_point b) {
        return std::chrono::duration<double, std::milli>(b - a).count();
    };
    double insert_ms = ms(t0, t1), search_ms = ms(t1, t2), delete_ms = ms(t2, t3);

    std::cout << "  [" << std::left << std::setw(18) << StrategyName(strategy) << "] n=" << n
              << std::right << "  insert=" << std::setw(8) << std::fixed << std::setprecision(1) << insert_ms
              << " ms (" << std::setw(8) << static_cast<long>(n / (insert_ms / 1000.0)) << " ops/s)"
              << "  search=" << std::setw(8) << search_ms << " ms (" << std::setw(8)
              << static_cast<long>(n / (search_ms / 1000.0)) << " ops/s)"
              << "  delete=" << std::setw(8) << delete_ms << " ms (" << std::setw(8)
              << static_cast<long>(n / (delete_ms / 1000.0)) << " ops/s)\n";

    (void)sink;
}

void DirectAddressStressTest(size_t universe, size_t operations, std::mt19937_64& rng) {
    DirectAddressTable<int> table(universe);
    std::vector<std::optional<int>> oracle(universe);

    std::uniform_int_distribution<size_t> key_dist(0, universe - 1);
    std::uniform_int_distribution<int> op_dist(0, 99);
    std::uniform_int_distribution<int> value_dist(-1000, 1000);

    for (size_t op = 0; op < operations; ++op) {
        size_t key = key_dist(rng);
        int roll = op_dist(rng);

        if (roll < 60) {
            int value = value_dist(rng);
            table.Insert(key, value);
            oracle[key] = value;
        } else if (roll < 85) {
            auto got = table.Search(key);
            Check(got == oracle[key], "DirectAddress: search mismatch at key " + std::to_string(key));
        } else {
            bool deleted = table.Delete(key);
            bool existed = oracle[key].has_value();
            Check(deleted == existed, "DirectAddress: delete result mismatch at key " + std::to_string(key));
            oracle[key].reset();
        }
    }

    size_t expected_size = 0;
    for (const auto& v : oracle) {
        if (v.has_value()) {
            ++expected_size;
        }
    }
    Check(table.Size() == expected_size, "DirectAddress: final size mismatch");

    std::cout << "  DirectAddressTable fuzz OK: " << operations << " ops over universe=" << universe
              << ", final size=" << table.Size() << "\n";
}

void UniversalHashDistributionTest(uint64_t m, uint64_t n, std::mt19937_64& rng) {
    UniversalHash h(m);
    std::vector<uint64_t> buckets(m, 0);
    std::uniform_int_distribution<uint64_t> x_dist;

    for (uint64_t i = 0; i < n; ++i) {
        uint64_t idx = h(x_dist(rng));
        ++buckets[idx];
    }

    double expected = static_cast<double>(n) / static_cast<double>(m);
    double chi_sq = 0.0;
    uint64_t max_bucket = 0;
    for (uint64_t count : buckets) {
        double diff = static_cast<double>(count) - expected;
        chi_sq += diff * diff / expected;
        max_bucket = std::max(max_bucket, count);
    }

    double threshold = m + 8.0 * std::sqrt(2.0 * m);
    Check(chi_sq < threshold * 3.0, "UniversalHash: chi-squared too high, distribution looks skewed");

    std::cout << "  UniversalHash distribution OK: m=" << m << " n=" << n << " chi_sq=" << std::fixed
              << std::setprecision(1) << chi_sq << " (threshold~" << threshold << "), max_bucket=" << max_bucket
              << " (expected~" << static_cast<long>(expected) << ")\n";
}

}

int main(int argc, char** argv) {
    size_t operations = 200000;
    uint64_t seed = 42;
    if (argc > 1) {
        operations = static_cast<size_t>(std::strtoull(argv[1], nullptr, 10));
    }
    if (argc > 2) {
        seed = std::strtoull(argv[2], nullptr, 10);
    }

    std::mt19937_64 rng(seed);
    std::cout << "Stress test: operations=" << operations << " seed=" << seed << "\n\n";

    const Strategy strategies[] = {Strategy::Chaining, Strategy::LinearProbing, Strategy::QuadraticProbing,
                                    Strategy::DoubleHashing};

    std::cout << "== 1) Correctness fuzzing vs std::unordered_map (int keys) ==\n";
    for (Strategy s : strategies) {
        std::mt19937_64 local_rng(seed + static_cast<int>(s) * 1000 + 1);
        std::function<int(std::mt19937_64&)> gen_key = [](std::mt19937_64& r) {
            std::uniform_int_distribution<int> d(0, 5000);
            return d(r);
        };
        FuzzAgainstOracle<int>(s, operations, 5000, local_rng, gen_key);
    }

    std::cout << "\n== 1b) Correctness fuzzing vs std::unordered_map (string keys) ==\n";
    for (Strategy s : strategies) {
        std::mt19937_64 local_rng(seed + static_cast<int>(s) * 1000 + 2);
        std::function<std::string(std::mt19937_64&)> gen_key = [](std::mt19937_64& r) {
            return RandomString(r, 6);
        };
        FuzzAgainstOracle<std::string>(s, operations, 5000, local_rng, gen_key);
    }

    std::cout << "\n== 2) Growth stress (monotonic insert, checking load factor & survival) ==\n";
    for (Strategy s : strategies) {
        GrowthStressTest(s, std::max<size_t>(operations, 50000));
    }

    std::cout << "\n== 3) Timing benchmark ==\n";
    for (size_t n : {10000ul, 100000ul, 1000000ul}) {
        for (Strategy s : strategies) {
            TimingBenchmark(s, n);
        }
        std::cout << "\n";
    }

    std::cout << "== 4) DirectAddressTable fuzzing ==\n";
    DirectAddressStressTest(/*universe=*/10000, operations, rng);

    std::cout << "\n== 5) UniversalHash distribution check ==\n";
    UniversalHashDistributionTest(/*m=*/1024, /*n=*/2000000, rng);
    UniversalHashDistributionTest(/*m=*/37, /*n=*/500000, rng);

    std::cout << "\n";
    if (g_failures == 0) {
        std::cout << "ALL STRESS TESTS PASSED.\n";
        return 0;
    }
    std::cerr << g_failures << " CHECK(S) FAILED.\n";
    return 1;
}
