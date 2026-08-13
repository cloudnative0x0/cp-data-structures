#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "LinkedList.hpp"

using cp::LinkedList;

int g_checks = 0;
int g_failures = 0;

void check(bool cond, const std::string& what) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::cerr << "FAILED: " << what << "\n";
    }
}

void assertSameContents(const LinkedList<int>& a, const std::list<int>& b,
                         const std::string& ctx) {
    check(a.size() == b.size(), ctx + ": size mismatch");
    if (a.size() != b.size()) return;

    if (!b.empty()) {
        check(a.front() == b.front(), ctx + ": front mismatch");
        check(a.back() == b.back(), ctx + ": back mismatch");
    }

    {
        auto ai = a.begin();
        auto bi = b.begin();
        std::size_t idx = 0;
        for (; ai != a.end() && bi != b.end(); ++ai, ++bi, ++idx) {
            if (*ai != *bi) {
                check(false, ctx + ": forward mismatch at index " + std::to_string(idx));
                break;
            }
        }
        check(ai == a.end(), ctx + ": forward iteration of `a` ran long/short");
        check(bi == b.end(), ctx + ": forward iteration of `b` ran long/short");
    }

    {
        auto ai = a.rbegin();
        auto bi = b.rbegin();
        std::size_t idx = 0;
        for (; ai != a.rend() && bi != b.rend(); ++ai, ++bi, ++idx) {
            if (*ai != *bi) {
                check(false, ctx + ": backward mismatch at index " + std::to_string(idx));
                break;
            }
        }
        check(ai == a.rend(), ctx + ": backward iteration of `a` ran long/short");
        check(bi == b.rend(), ctx + ": backward iteration of `b` ran long/short");
    }

    check(static_cast<std::size_t>(std::distance(a.begin(), a.end())) == a.size(),
              ctx + ": distance(begin,end) != size()");
}

template <typename Iter>
Iter advanced(Iter it, long k) {
    std::advance(it, k);
    return it;
}

void test_basic_push_pop() {
    LinkedList<int> l;
    check(l.empty(), "l.empty()");
    l.push_back(1);
    l.push_back(2);
    l.push_front(0);

    std::vector<int> got(l.begin(), l.end());
    check((got == std::vector<int>{0, 1, 2}), "(got == std::vector<int>{0, 1, 2})");
    check(l.front() == 0, "l.front() == 0");
    check(l.back() == 2, "l.back() == 2");

    l.pop_front();
    l.pop_back();
    check(l.size() == 1, "l.size() == 1");
    check(l.front() == 1, "l.front() == 1");
    l.pop_back();
    check(l.empty(), "l.empty()");
}

void test_copy_and_move() {
    LinkedList<int> a{1, 2, 3, 4, 5};
    LinkedList<int> b = a;
    check(a == b, "a == b");
    b.push_back(6);
    check(a != b, "a != b");

    LinkedList<int> c = std::move(b);
    check((c == LinkedList<int>{1, 2, 3, 4, 5, 6}), "(c == LinkedList<int>{1, 2, 3, 4, 5, 6})");
    check(b.empty(), "b.empty()");

    LinkedList<int> d;
    d.push_back(42);
    d = a;
    check(d == a, "d == a");

    LinkedList<int> e;
    e.push_back(-1);
    e = std::move(c);
    check((e == LinkedList<int>{1, 2, 3, 4, 5, 6}), "(e == LinkedList<int>{1, 2, 3, 4, 5, 6})");
    check(c.empty(), "c.empty()");
}

void test_self_assignment_and_self_swap() {
    LinkedList<int> a{1, 2, 3};

    a = a;
    check((a == LinkedList<int>{1, 2, 3}), "(a == LinkedList<int>{1, 2, 3})");

    a.swap(a);
    check((a == LinkedList<int>{1, 2, 3}), "(a == LinkedList<int>{1, 2, 3})");

    a = std::move(a);
    check(a.size() <= 3, "a.size() <= 3");

    std::size_t cnt = 0;
    for (auto it = a.begin(); it != a.end() && cnt <= 10; ++it) ++cnt;
    check(cnt == a.size(), "cnt == a.size()");
}

void test_erase_and_ranges() {
    LinkedList<int> a{1, 2, 3, 4, 5};
    auto it = a.begin();
    ++it;
    it = a.erase(it);
    check(*it == 3, "*it == 3");
    check((a == LinkedList<int>{1, 3, 4, 5}), "(a == LinkedList<int>{1, 3, 4, 5})");

    auto first = a.begin();
    auto last = a.end();
    auto res = a.erase(first, last);
    check(a.empty(), "a.empty()");
    check(res == a.end(), "res == a.end()");

    LinkedList<int> b{1, 2, 3};
    auto bit = b.begin();
    ++bit;
    auto same = b.erase(bit, bit);
    check(*same == 2, "*same == 2");
    check((b == LinkedList<int>{1, 2, 3}), "(b == LinkedList<int>{1, 2, 3})");
}

void test_insert_variants() {
    LinkedList<int> a{1, 5};
    auto pos = a.begin();
    ++pos;
    a.insert(pos, 3);
    check((a == LinkedList<int>{1, 3, 5}), "(a == LinkedList<int>{1, 3, 5})");

    a.insert(a.end(), std::size_t{3}, 9);
    check((a == LinkedList<int>{1, 3, 5, 9, 9, 9}), "(a == LinkedList<int>{1, 3, 5, 9, 9, 9})");

    std::vector<int> src{100, 200, 300};
    a.insert(a.begin(), src.begin(), src.end());
    check((a == LinkedList<int>{100, 200, 300, 1, 3, 5, 9, 9, 9}), "(a == LinkedList<int>{100, 200, 300, 1, 3, 5, 9, 9, 9})");

    LinkedList<int> b{7, 8};
    auto it = b.begin();
    ++it;
    auto r = b.insert(it, std::size_t{0}, 42);
    check(*r == 8, "*r == 8");
    check((b == LinkedList<int>{7, 8}), "(b == LinkedList<int>{7, 8})");
}

void test_remove_remove_if_unique() {
    LinkedList<int> a{1, 2, 2, 3, 3, 3, 4, 1};
    auto removed = a.remove(3);
    check(removed == 3, "removed == 3");
    check((a == LinkedList<int>{1, 2, 2, 4, 1}), "(a == LinkedList<int>{1, 2, 2, 4, 1})");

    auto removed_if = a.remove_if([](int v) { return v % 2 == 0; });
    check(removed_if == 3, "removed_if == 3");
    check((a == LinkedList<int>{1, 1}), "(a == LinkedList<int>{1, 1})");

    LinkedList<int> b{1, 1, 2, 2, 2, 3, 1, 1};
    auto u = b.unique();
    check(u == 4, "u == 4");
    check((b == LinkedList<int>{1, 2, 3, 1}), "(b == LinkedList<int>{1, 2, 3, 1})");

    LinkedList<int> empty1;
    check(empty1.unique() == 0, "empty1.unique() == 0");
    LinkedList<int> one{5};
    check(one.unique() == 0, "one.unique() == 0");
}

void test_sort_stability_and_reverse() {

    struct Pair {
        int key;
        int orig;
    };
    LinkedList<Pair> l;
    std::vector<Pair> ref;
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> keyDist(0, 4);
    for (int i = 0; i < 500; ++i) {
        Pair p{keyDist(rng), i};
        l.push_back(p);
        ref.push_back(p);
    }
    l.sort([](const Pair& x, const Pair& y) { return x.key < y.key; });
    std::stable_sort(ref.begin(), ref.end(),
                      [](const Pair& x, const Pair& y) { return x.key < y.key; });

    std::vector<Pair> got(l.begin(), l.end());
    check(got.size() == ref.size(), "sort: size mismatch");
    bool same = std::equal(got.begin(), got.end(), ref.begin(), ref.end(),
                            [](const Pair& x, const Pair& y) {
                                return x.key == y.key && x.orig == y.orig;
                            });
    check(same, "merge sort is not stable (or produced wrong order)");

    LinkedList<int> r{1, 2, 3, 4, 5};
    r.reverse();
    check((r == LinkedList<int>{5, 4, 3, 2, 1}), "(r == LinkedList<int>{5, 4, 3, 2, 1})");
    r.reverse();
    check((r == LinkedList<int>{1, 2, 3, 4, 5}), "(r == LinkedList<int>{1, 2, 3, 4, 5})");

    LinkedList<int> single{1};
    single.reverse();
    check((single == LinkedList<int>{1}), "(single == LinkedList<int>{1})");

    LinkedList<int> emptyList;
    emptyList.reverse();
    check(emptyList.empty(), "emptyList.empty()");
}

void test_splice_whole_and_element_and_range() {
    std::cout << "   [splice] whole\n" << std::flush;

    {
        LinkedList<int> a{1, 2, 3};
        LinkedList<int> b{10, 20};
        auto pos = a.begin();
        ++pos;
        a.splice(pos, b);
        check((a == LinkedList<int>{1, 10, 20, 2, 3}), "(a == LinkedList<int>{1, 10, 20, 2, 3})");
        check(b.empty(), "b.empty()");
        check(b.size() == 0, "b.size() == 0");
    }

    std::cout << "   [splice] single elem cross-list\n" << std::flush;

    {
        LinkedList<int> a{1, 2, 3};
        LinkedList<int> b{10, 20, 30};
        auto it = b.begin();
        ++it;
        a.splice(a.end(), b, it);
        check((a == LinkedList<int>{1, 2, 3, 20}), "(a == LinkedList<int>{1, 2, 3, 20})");
        check((b == LinkedList<int>{10, 30}), "(b == LinkedList<int>{10, 30})");
    }

    std::cout << "   [splice] range cross-list\n" << std::flush;

    {
        LinkedList<int> a{1, 2};
        LinkedList<int> b{10, 20, 30, 40};
        auto f = b.begin();
        ++f;
        auto l = f;
        std::advance(l, 2);
        a.splice(a.begin(), b, f, l);
        check((a == LinkedList<int>{20, 30, 1, 2}), "(a == LinkedList<int>{20, 30, 1, 2})");
        check((b == LinkedList<int>{10, 40}), "(b == LinkedList<int>{10, 40})");
    }

    std::cout << "   [splice] same-list range move\n" << std::flush;

    {
        LinkedList<int> a{1, 2, 3, 4, 5};
        auto f = a.begin();
        std::advance(f, 1);
        auto l = a.begin();
        std::advance(l, 3);
        a.splice(a.end(), a, f, l);
        check((a == LinkedList<int>{1, 4, 5, 2, 3}), "(a == LinkedList<int>{1, 4, 5, 2, 3})");
        check(a.size() == 5, "a.size() == 5");
    }

    std::cout << "   [splice] empty range self\n" << std::flush;

    {
        LinkedList<int> a{1, 2, 3};
        auto it = a.begin();
        ++it;
        a.splice(a.end(), a, it, it);
        check((a == LinkedList<int>{1, 2, 3}), "(a == LinkedList<int>{1, 2, 3})");
    }

    std::cout << "   [splice] self-splice pos==it  (KNOWN BUG — run isolated/bounded)\n" << std::flush;
    {

#ifdef HAVE_LSAN_INTERFACE
        __lsan::ScopedDisabler lsanDisabler;
#endif
        auto* a = new LinkedList<int>{1, 2, 3};
        auto it = a->begin();
        ++it;
        a->splice(it, *a, it);

        bool sizeOk = (a->size() == 3);
        check(sizeOk, "self-splice(pos==it): size() changed (expected no-op)");

        std::vector<int> got;
        auto cur = a->begin();
        std::size_t steps = 0;
        constexpr std::size_t kStepLimit = 16;
        while (cur != a->end() && steps < kStepLimit) {
            got.push_back(*cur);
            ++cur;
            ++steps;
        }
        bool structureOk = (steps < kStepLimit) && (got == std::vector<int>{1, 2, 3});
        check(structureOk,
                  "BUG CONFIRMED: splice(pos, list, it) with pos == it corrupts the "
                  "list into a detached self-cycle instead of being a no-op "
                  "(see comment above for the exact pointer-aliasing bug in "
                  "LinkedList::splice). Do not call splice with a destination "
                  "iterator that lies inside the moved range, including pos == first.");

    }

    {
        LinkedList<int> a{1, 2, 3};
        auto it = a.begin();
        auto pos = it; ++pos; ++pos;

        auto posOn2 = a.begin(); ++posOn2;
        a.splice(posOn2, a, it);
        check(a.size() == 3, "self-splice(next, it) corrupted size");
        std::vector<int> got(a.begin(), a.end());
        check((got == std::vector<int>{1, 2, 3}),
                  "self-splice(pos==next(it)) must be a no-op");
    }
}

void test_resize() {
    LinkedList<int> a{1, 2, 3};
    a.resize(5, -1);
    check((a == LinkedList<int>{1, 2, 3, -1, -1}), "(a == LinkedList<int>{1, 2, 3, -1, -1})");
    a.resize(2);
    check((a == LinkedList<int>{1, 2}), "(a == LinkedList<int>{1, 2})");
    a.resize(0);
    check(a.empty(), "a.empty()");
    a.resize(3, 7);
    check((a == LinkedList<int>{7, 7, 7}), "(a == LinkedList<int>{7, 7, 7})");
}

void test_const_iterator_conversion_and_reverse_iterators() {
    LinkedList<int> a{1, 2, 3};
    LinkedList<int>::const_iterator cit = a.begin();
    check(*cit == 1, "*cit == 1");

    std::vector<int> rev(a.rbegin(), a.rend());
    check((rev == std::vector<int>{3, 2, 1}), "(rev == std::vector<int>{3, 2, 1})");

    const LinkedList<int>& ca = a;
    std::vector<int> crev(ca.crbegin(), ca.crend());
    check((crev == std::vector<int>{3, 2, 1}), "(crev == std::vector<int>{3, 2, 1})");
}

struct Model {
    LinkedList<int> lst;
    std::list<int> ref;

    void check(const std::string& ctx) { assertSameContents(lst, ref, ctx); }
};

void fuzz_run(std::uint32_t seed, int iterations) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> valueDist(-1000, 1000);

    Model A, B;

    auto randPos = [&](std::size_t sz) -> long {
        if (sz == 0) return 0;
        std::uniform_int_distribution<long> d(0, static_cast<long>(sz));
        return d(rng);
    };

    for (int iter = 0; iter < iterations; ++iter) {
        std::uniform_int_distribution<int> opDist(0, 21);
        int op = opDist(rng);
        Model& M = (rng() % 5 == 0) ? B : A;

        switch (op) {
            case 0: {
                int v = valueDist(rng);
                M.lst.push_back(v);
                M.ref.push_back(v);
                break;
            }
            case 1: {
                int v = valueDist(rng);
                M.lst.push_front(v);
                M.ref.push_front(v);
                break;
            }
            case 2: {
                if (!M.ref.empty()) { M.lst.pop_back(); M.ref.pop_back(); }
                break;
            }
            case 3: {
                if (!M.ref.empty()) { M.lst.pop_front(); M.ref.pop_front(); }
                break;
            }
            case 4: {
                long k = randPos(M.ref.size());
                int v = valueDist(rng);
                M.lst.insert(advanced(M.lst.begin(), k), v);
                M.ref.insert(advanced(M.ref.begin(), k), v);
                break;
            }
            case 5: {
                if (!M.ref.empty()) {
                    long k = randPos(M.ref.size() - 1);
                    M.lst.erase(advanced(M.lst.begin(), k));
                    M.ref.erase(advanced(M.ref.begin(), k));
                }
                break;
            }
            case 6: {
                if (!M.ref.empty()) {
                    long sz = static_cast<long>(M.ref.size());
                    long i = randPos(sz - 1);
                    long j = i + (rng() % (sz - i + 1));
                    M.lst.erase(advanced(M.lst.begin(), i), advanced(M.lst.begin(), j));
                    M.ref.erase(advanced(M.ref.begin(), i), advanced(M.ref.begin(), j));
                }
                break;
            }
            case 7: {
                M.lst.clear();
                M.ref.clear();
                break;
            }
            case 8: {
                std::uniform_int_distribution<std::size_t> cd(0, 30);
                std::size_t cnt = cd(rng);
                int v = valueDist(rng);
                M.lst.resize(cnt, v);
                M.ref.resize(cnt, v);
                break;
            }
            case 9: {
                M.lst.reverse();
                M.ref.reverse();
                break;
            }
            case 10: {
                M.lst.sort();
                M.ref.sort();
                break;
            }
            case 11: {
                M.lst.unique();
                M.ref.unique();
                break;
            }
            case 12: {
                int v = valueDist(rng);
                auto r1 = M.lst.remove(v);

                std::size_t before = M.ref.size();
                M.ref.remove(v);
                std::size_t r2 = before - M.ref.size();
                check(r1 == r2, "remove() count mismatch");
                break;
            }
            case 13: {
                auto r1 = M.lst.remove_if([](int v) { return v % 3 == 0; });
                std::size_t before = M.ref.size();
                M.ref.remove_if([](int v) { return v % 3 == 0; });
                std::size_t r2 = before - M.ref.size();
                check(r1 == r2, "remove_if() count mismatch");
                break;
            }
            case 14: {
                if (&M == &A && !B.ref.empty()) {
                    long k = randPos(A.ref.size());
                    A.lst.splice(advanced(A.lst.begin(), k), B.lst);
                    A.ref.splice(advanced(A.ref.begin(), k), B.ref);
                } else if (&M == &B && !A.ref.empty()) {
                    long k = randPos(B.ref.size());
                    B.lst.splice(advanced(B.lst.begin(), k), A.lst);
                    B.ref.splice(advanced(B.ref.begin(), k), A.ref);
                }
                break;
            }
            case 15: {
                Model& Src = (&M == &A) ? B : A;
                if (!Src.ref.empty()) {
                    long srcK = randPos(Src.ref.size() - 1);
                    long dstK = randPos(M.ref.size());
                    M.lst.splice(advanced(M.lst.begin(), dstK), Src.lst,
                                 advanced(Src.lst.begin(), srcK));
                    M.ref.splice(advanced(M.ref.begin(), dstK), Src.ref,
                                 advanced(Src.ref.begin(), srcK));
                }
                break;
            }
            case 16: {
                Model& Src = (&M == &A) ? B : A;
                if (!Src.ref.empty()) {
                    long sz = static_cast<long>(Src.ref.size());
                    long i = randPos(sz - 1);
                    long j = i + (rng() % (sz - i + 1));
                    long dstK = randPos(M.ref.size());
                    M.lst.splice(advanced(M.lst.begin(), dstK), Src.lst,
                                 advanced(Src.lst.begin(), i), advanced(Src.lst.begin(), j));
                    M.ref.splice(advanced(M.ref.begin(), dstK), Src.ref,
                                 advanced(Src.ref.begin(), i), advanced(Src.ref.begin(), j));
                }
                break;
            }
            case 17: {
                if (M.ref.size() >= 2) {
                    long sz = static_cast<long>(M.ref.size());
                    long i = randPos(sz - 1);
                    long j = i + (rng() % (sz - i + 1));

                    long dstK;
                    if (i == 0 && j == sz) break;
                    if (rng() % 2 == 0 && i > 0) dstK = randPos(i - 1);
                    else dstK = j + (rng() % (sz - j + 1));
                    M.lst.splice(advanced(M.lst.begin(), dstK), M.lst,
                                 advanced(M.lst.begin(), i), advanced(M.lst.begin(), j));
                    M.ref.splice(advanced(M.ref.begin(), dstK), M.ref,
                                 advanced(M.ref.begin(), i), advanced(M.ref.begin(), j));
                }
                break;
            }
            case 18: {
                A.lst.swap(B.lst);
                A.ref.swap(B.ref);
                break;
            }
            case 19: {
                if (&M == &A) { A.lst = B.lst; A.ref = B.ref; }
                else { B.lst = A.lst; B.ref = A.ref; }
                break;
            }
            case 20: {
                if (&M == &A) { A.lst = std::move(B.lst); A.ref = std::move(B.ref); }
                else { B.lst = std::move(A.lst); B.ref = std::move(A.ref); }
                break;
            }
            case 21: {
                int v = valueDist(rng);
                if (rng() % 2 == 0) { M.lst.emplace_back(v); M.ref.emplace_back(v); }
                else { M.lst.emplace_front(v); M.ref.emplace_front(v); }
                break;
            }
        }

        if (iter % 25 == 0) {
            A.check("A @ iter " + std::to_string(iter));
            B.check("B @ iter " + std::to_string(iter));
        }
    }

    A.check("A final");
    B.check("B final");
}

int main(int argc, char** argv) {
    std::uint32_t seed = argc > 1 ? static_cast<std::uint32_t>(std::stoul(argv[1])) : 1u;
    int iterations = argc > 2 ? std::stoi(argv[2]) : 20000;

    std::cout << "== deterministic edge-case tests ==\n";
    std::cout << "-> test_basic_push_pop\n" << std::flush; test_basic_push_pop();
    std::cout << "-> test_copy_and_move\n" << std::flush; test_copy_and_move();
    std::cout << "-> test_self_assignment_and_self_swap\n" << std::flush; test_self_assignment_and_self_swap();
    std::cout << "-> test_erase_and_ranges\n" << std::flush; test_erase_and_ranges();
    std::cout << "-> test_insert_variants\n" << std::flush; test_insert_variants();
    std::cout << "-> test_remove_remove_if_unique\n" << std::flush; test_remove_remove_if_unique();
    std::cout << "-> test_sort_stability_and_reverse\n" << std::flush; test_sort_stability_and_reverse();
    std::cout << "-> test_splice_whole_and_element_and_range\n" << std::flush; test_splice_whole_and_element_and_range();
    std::cout << "-> test_resize\n" << std::flush; test_resize();
    std::cout << "-> test_const_iterator_conversion_and_reverse_iterators\n" << std::flush; test_const_iterator_conversion_and_reverse_iterators();
    std::cout << "-> deterministic tests done\n" << std::flush;
    std::cout << "  checks so far: " << g_checks
              << ", failures: " << g_failures << "\n";

    std::cout << "== differential fuzz test vs std::list ==\n";
    std::cout << "  seed=" << seed << " iterations=" << iterations << "\n";
    fuzz_run(seed, iterations);

    for (std::uint32_t s = seed + 1; s < seed + 5; ++s) {
        fuzz_run(s, std::max(2000, iterations / 4));
    }

    std::cout << "\n== summary ==\n";
    std::cout << "checks total:  " << g_checks << "\n";
    std::cout << "failures:      " << g_failures << "\n";

    if (g_failures > 0) {
        std::cout << "RESULT: FAIL\n";
        return 1;
    }
    std::cout << "RESULT: OK\n";
    return 0;
}