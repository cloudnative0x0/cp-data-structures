#ifndef CP_DATA_STRUCTURES_LINKEDLIST_HPP
#define CP_DATA_STRUCTURES_LINKEDLIST_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace cp {

template <typename T>
class LinkedList {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;

private:
    struct NodeBase {
        NodeBase* prev;
        NodeBase* next;
    };

    struct Node : NodeBase {
        T value;
        template <typename... Args>
        explicit Node(Args&&... args)
            : NodeBase{nullptr, nullptr}, value(std::forward<Args>(args)...) {}
    };

    static Node* asNode(NodeBase* b) noexcept { return static_cast<Node*>(b); }
    static const Node* asNode(const NodeBase* b) noexcept { return static_cast<const Node*>(b); }

    class NodePool {
        static constexpr std::size_t kChunkNodes = 1024;

        struct Chunk {
            alignas(Node) unsigned char storage[kChunkNodes * sizeof(Node)];
            Chunk* next;
        };

        Chunk* chunks_       = nullptr;
        std::size_t used_    = kChunkNodes;
        NodeBase* freeList_  = nullptr;

        void* rawAlloc() {
            if (freeList_) {
                NodeBase* slot = freeList_;
                freeList_ = freeList_->next;
                return slot;
            }
            if (used_ == kChunkNodes) {
                Chunk* fresh = new Chunk();
                fresh->next = chunks_;
                chunks_ = fresh;
                used_ = 0;
            }
            void* slot = chunks_->storage + used_ * sizeof(Node);
            ++used_;

            return slot;
        }

    public:
        NodePool() = default;
        NodePool(const NodePool&) = delete;
        NodePool& operator=(const NodePool&) = delete;

        ~NodePool() {
            Chunk* c = chunks_;
            while (c) {
                Chunk* nxt = c->next;
                delete c;
                c = nxt;
            }
        }

        template <typename... Args>
        Node* create(Args&&... args) {
            void* raw = rawAlloc();
            return ::new (raw) Node(std::forward<Args>(args)...);
        }

        void destroy(Node* n) noexcept {
            n->~Node();
            NodeBase* base = n;
            base->next = freeList_;
            freeList_ = base;
        }
    };

    // Common pool for each instance type T
    static NodePool& pool() {
        static NodePool instance;
        return instance;
    }

    NodeBase sentinel_{};
    size_type size_ = 0;

    void initSentinel() noexcept {
        sentinel_.prev = &sentinel_;
        sentinel_.next = &sentinel_;
    }

    static void linkBefore(NodeBase* pos, NodeBase* node) noexcept {
        node->prev = pos->prev;
        node->next = pos;
        pos->prev->next = node;
        pos->prev = node;
    }

    static void unlink(NodeBase* node) noexcept {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void stealFrom(LinkedList& other) noexcept {
        if (other.empty()) return;
        sentinel_.next = other.sentinel_.next;
        sentinel_.prev = other.sentinel_.prev;
        sentinel_.next->prev = &sentinel_;
        sentinel_.prev->next = &sentinel_;
        size_ = other.size_;
        other.initSentinel();
        other.size_ = 0;
    }

public:
    // Iterators — bidirectional
    class const_iterator;

    class iterator {
        friend class LinkedList;
        friend class const_iterator;
        NodeBase* node_;
        explicit iterator(NodeBase* n) noexcept : node_(n) {}

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        iterator() noexcept : node_(nullptr) {}

        reference operator*() const { return asNode(node_)->value; }
        pointer operator->() const { return std::addressof(asNode(node_)->value); }

        iterator& operator++() { node_ = node_->next; return *this; }
        iterator operator++(int) { iterator tmp(*this); ++(*this); return tmp; }
        iterator& operator--() { node_ = node_->prev; return *this; }
        iterator operator--(int) { iterator tmp(*this); --(*this); return tmp; }

        bool operator==(const iterator& o) const noexcept { return node_ == o.node_; }
        bool operator!=(const iterator& o) const noexcept { return node_ != o.node_; }
    };

    class const_iterator {
        friend class LinkedList;
        const NodeBase* node_;
        explicit const_iterator(const NodeBase* n) noexcept : node_(n) {}

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        const_iterator() noexcept : node_(nullptr) {}
        const_iterator(iterator it) noexcept : node_(it.node_) {}

        reference operator*() const { return asNode(node_)->value; }
        pointer operator->() const { return std::addressof(asNode(node_)->value); }

        const_iterator& operator++() { node_ = node_->next; return *this; }
        const_iterator operator++(int) { const_iterator tmp(*this); ++(*this); return tmp; }
        const_iterator& operator--() { node_ = node_->prev; return *this; }
        const_iterator operator--(int) { const_iterator tmp(*this); --(*this); return tmp; }

        bool operator==(const const_iterator& o) const noexcept { return node_ == o.node_; }
        bool operator!=(const const_iterator& o) const noexcept { return node_ != o.node_; }
    };

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors
    LinkedList() noexcept { initSentinel(); }

    LinkedList(std::initializer_list<T> init) : LinkedList() {
        for (const auto& v : init) push_back(v);
    }

    explicit LinkedList(size_type n, const T& value = T()) : LinkedList() {
        for (size_type i = 0; i < n; ++i) push_back(value);
    }

    template <typename InputIt,
              typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    LinkedList(InputIt first, InputIt last) : LinkedList() {
        for (; first != last; ++first) push_back(*first);
    }

    LinkedList(const LinkedList& other) : LinkedList() {
        for (const auto& v : other) push_back(v);
    }

    LinkedList(LinkedList&& other) noexcept : LinkedList() {
        stealFrom(other);
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            LinkedList tmp(other);
            swap(tmp);
        }

        return *this;
    }

    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            stealFrom(other);
        }

        return *this;
    }

    LinkedList& operator=(std::initializer_list<T> init) {
        LinkedList tmp(init);
        swap(tmp);

        return *this;
    }

    ~LinkedList() { clear(); }

    // Interation
    iterator begin() noexcept { return iterator(sentinel_.next); }
    iterator end() noexcept { return iterator(&sentinel_); }
    const_iterator begin() const noexcept { return const_iterator(sentinel_.next); }
    const_iterator end() const noexcept { return const_iterator(&sentinel_); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    const_reverse_iterator crend() const noexcept { return rend(); }

    // Capacity
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    size_type size() const noexcept { return size_; }
    size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

    // Access to the elements
    reference front() { return asNode(sentinel_.next)->value; }
    const_reference front() const { return asNode(sentinel_.next)->value; }
    reference back() { return asNode(sentinel_.prev)->value; }
    const_reference back() const { return asNode(sentinel_.prev)->value; }

    // Modificators
    void clear() noexcept {
        NodeBase* cur = sentinel_.next;
        while (cur != &sentinel_) {
            NodeBase* nxt = cur->next;
            pool().destroy(asNode(cur));
            cur = nxt;
        }
        initSentinel();
        size_ = 0;
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        Node* node = pool().create(std::forward<Args>(args)...);
        linkBefore(const_cast<NodeBase*>(pos.node_), node);
        ++size_;

        return iterator(node);
    }

    iterator insert(const_iterator pos, const T& value) { return emplace(pos, value); }
    iterator insert(const_iterator pos, T&& value) { return emplace(pos, std::move(value)); }

    iterator insert(const_iterator pos, size_type n, const T& value) {
        iterator first = iterator(const_cast<NodeBase*>(pos.node_));
        bool firstSet = false;
        for (size_type i = 0; i < n; ++i) {
            iterator it = emplace(pos, value);
            if (!firstSet) { first = it; firstSet = true; }
        }

        return first;
    }

    template <typename InputIt,
              typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        iterator result = iterator(const_cast<NodeBase*>(pos.node_));
        bool firstSet = false;
        for (; first != last; ++first) {
            iterator it = emplace(pos, *first);
            if (!firstSet) { result = it; firstSet = true; }
        }

        return result;
    }

    iterator erase(const_iterator pos) {
        NodeBase* n = const_cast<NodeBase*>(pos.node_);
        NodeBase* nxt = n->next;
        unlink(n);
        pool().destroy(asNode(n));
        --size_;

        return iterator(nxt);
    }

    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) first = erase(first);

        return iterator(const_cast<NodeBase*>(last.node_));
    }

    void push_back(const T& value) { emplace(end(), value); }
    void push_back(T&& value) { emplace(end(), std::move(value)); }
    template <typename... Args>
    reference emplace_back(Args&&... args) { return *emplace(end(), std::forward<Args>(args)...); }
    void pop_back() { erase(iterator(sentinel_.prev)); }

    void push_front(const T& value) { emplace(begin(), value); }
    void push_front(T&& value) { emplace(begin(), std::move(value)); }
    template <typename... Args>
    reference emplace_front(Args&&... args) { return *emplace(begin(), std::forward<Args>(args)...); }
    void pop_front() { erase(begin()); }

    void resize(size_type count) { resize(count, T()); }
    void resize(size_type count, const T& value) {
        if (count < size_) { while (size_ > count) pop_back(); }
        else { while (size_ < count) push_back(value); }
    }

    void swap(LinkedList& other) noexcept {
        if (this == &other) return;

        NodeBase* aNext = sentinel_.next;
        NodeBase* aPrev = sentinel_.prev;
        NodeBase* bNext = other.sentinel_.next;
        NodeBase* bPrev = other.sentinel_.prev;
        const bool aEmpty = empty();
        const bool bEmpty = other.empty();

        if (aEmpty) {
            other.sentinel_.next = &other.sentinel_;
            other.sentinel_.prev = &other.sentinel_;
        } else {
            other.sentinel_.next = aNext;
            other.sentinel_.prev = aPrev;
            aNext->prev = &other.sentinel_;
            aPrev->next = &other.sentinel_;
        }

        if (bEmpty) {
            sentinel_.next = &sentinel_;
            sentinel_.prev = &sentinel_;
        } else {
            sentinel_.next = bNext;
            sentinel_.prev = bPrev;
            bNext->prev = &sentinel_;
            bPrev->next = &sentinel_;
        }

        std::swap(size_, other.size_);
    }

    // splice — honest O(1)
    void splice(const_iterator pos, LinkedList& other) noexcept {
        splice(pos, other, other.begin(), other.end());
    }

    void splice(const_iterator pos, LinkedList& other, const_iterator it) noexcept {
        const_iterator next = it; ++next;
        splice(pos, other, it, next);
    }

    void splice(const_iterator pos, LinkedList& other,
                const_iterator first, const_iterator last) noexcept {
        if (first == last) return;

        NodeBase* posN = const_cast<NodeBase*>(pos.node_);
        NodeBase* firstN = const_cast<NodeBase*>(first.node_);
        NodeBase* lastExclN = const_cast<NodeBase*>(last.node_);
        NodeBase* lastInclN = lastExclN->prev;

        if (&other == this) {
            for (NodeBase* p = firstN; p != lastExclN; p = p->next) {
                if (posN == p) return;
            }
        }

        size_type moved = 0;
        if (&other != this) {
            for (NodeBase* p = firstN; p != lastExclN; p = p->next) ++moved;
        }

        firstN->prev->next = lastExclN;
        lastExclN->prev = firstN->prev;

        NodeBase* before = posN->prev;
        before->next = firstN;
        firstN->prev = before;
        lastInclN->next = posN;
        posN->prev = lastInclN;

        if (&other != this) {
            size_ += moved;
            other.size_ -= moved;
        }
    }

    // Algorithms
    void reverse() noexcept {
        NodeBase* cur = &sentinel_;
        do {
            NodeBase* nxt = cur->next;
            cur->next = cur->prev;
            cur->prev = nxt;
            cur = nxt;
        } while (cur != &sentinel_);
    }

    template <typename UnaryPred>
    size_type remove_if(UnaryPred pred) {
        size_type removed = 0;
        for (iterator it = begin(); it != end(); ) {
            if (pred(*it)) { it = erase(it); ++removed; }
            else { ++it; }
        }

        return removed;
    }

    size_type remove(const T& value) {
        return remove_if([&value](const T& v) { return v == value; });
    }

    template <typename BinaryPred = std::equal_to<T>>
    size_type unique(BinaryPred pred = BinaryPred()) {
        if (size_ < 2) return 0;
        size_type removed = 0;
        iterator it = begin();
        iterator nxt = it; ++nxt;

        while (nxt != end()) {
            if (pred(*it, *nxt)) { nxt = erase(nxt); ++removed; }
            else { it = nxt; ++nxt; }
        }

        return removed;
    }

    template <typename Compare = std::less<T>>
    void sort(Compare comp = Compare()) {
        if (size_ < 2) return;
        sentinel_.prev->next = nullptr;
        NodeBase* head = sentinel_.next;
        head = mergeSort(head, comp);

        NodeBase* prev = &sentinel_;
        NodeBase* cur = head;
        while (cur) {
            prev->next = cur;
            cur->prev = prev;
            prev = cur;
            cur = cur->next;
        }
        prev->next = &sentinel_;
        sentinel_.prev = prev;
    }

    friend bool operator==(const LinkedList& a, const LinkedList& b) {
        if (a.size_ != b.size_) return false;

        return std::equal(a.begin(), a.end(), b.begin());
    }
    friend bool operator!=(const LinkedList& a, const LinkedList& b) { return !(a == b); }

private:
    template <typename Compare>
    static NodeBase* mergeSort(NodeBase* head, Compare& comp) {
        if (!head || !head->next) return head;

        NodeBase* slow = head;
        NodeBase* fast = head->next;
        while (fast && fast->next) { slow = slow->next; fast = fast->next->next; }
        NodeBase* rightHead = slow->next;
        slow->next = nullptr;

        NodeBase* left = mergeSort(head, comp);
        NodeBase* right = mergeSort(rightHead, comp);

        return merge(left, right, comp);
    }

    template <typename Compare>
    static NodeBase* merge(NodeBase* a, NodeBase* b, Compare& comp) {
        NodeBase dummy{nullptr, nullptr};
        NodeBase* tail = &dummy;
        while (a && b) {
            if (comp(asNode(b)->value, asNode(a)->value)) { tail->next = b; b = b->next; }
            else { tail->next = a; a = a->next; }
            tail = tail->next;
        }
        tail->next = a ? a : b;

        return dummy.next;
    }
};

template <typename T>
void swap(LinkedList<T>& a, LinkedList<T>& b) noexcept { a.swap(b); }

}

#endif // CP_DATA_STRUCTURES_LINKEDLIST_HPP