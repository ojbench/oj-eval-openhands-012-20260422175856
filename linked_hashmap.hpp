/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */
    
template<
        class Key,
        class T,
        class Hash = std::hash<Key>, 
        class Equal = std::equal_to<Key>
    > class linked_hashmap {
public:
    typedef pair<const Key, T> value_type;
 
    struct Node {
        value_type kv;
        Node *list_prev;
        Node *list_next;
        Node *hash_next;
        Node(const value_type &v) : kv(v), list_prev(nullptr), list_next(nullptr), hash_next(nullptr) {}
        Node(const Key &k) : kv(k, T()), list_prev(nullptr), list_next(nullptr), hash_next(nullptr) {}
    };

    class const_iterator;
    class iterator {
    private:
        using map_type = linked_hashmap;
        friend class linked_hashmap;
        friend class const_iterator;
        map_type *mp;
        Node *node;
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::output_iterator_tag;

        iterator() : mp(nullptr), node(nullptr) {}
        iterator(map_type *m, Node *n) : mp(m), node(n) {}
        iterator(const iterator &other) : mp(other.mp), node(other.node) {}
        iterator operator++(int) { iterator tmp(*this); ++(*this); return tmp; }
        iterator & operator++() {
            if (mp == nullptr) throw invalid_iterator();
            if (node == nullptr) throw invalid_iterator();
            node = node->list_next;
            return *this;
        }
        iterator operator--(int) { iterator tmp(*this); --(*this); return tmp; }
        iterator & operator--() {
            if (mp == nullptr) throw invalid_iterator();
            if (node == nullptr) { node = mp->tail; if (node == nullptr) throw invalid_iterator(); return *this; }
            if (node->list_prev == nullptr) throw invalid_iterator();
            node = node->list_prev;
            return *this;
        }
        value_type & operator*() const { if (node == nullptr) throw invalid_iterator(); return node->kv; }
        bool operator==(const iterator &rhs) const { return mp == rhs.mp && node == rhs.node; }
        bool operator==(const const_iterator &rhs) const { return mp == rhs.mp && node == rhs.node; }
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
        bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
        value_type* operator->() const noexcept { return &(node->kv); }
    };
 
    class const_iterator {
    private:
        using map_type = linked_hashmap;
        friend class linked_hashmap;
        friend class iterator;
        const map_type *mp;
        const Node *node;
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::output_iterator_tag;
        const_iterator() : mp(nullptr), node(nullptr) {}
        const_iterator(const map_type *m, const Node *n) : mp(m), node(n) {}
        const_iterator(const const_iterator &other) : mp(other.mp), node(other.node) {}
        const_iterator(const iterator &other) : mp(other.mp), node(other.node) {}
        const_iterator operator++(int) { const_iterator tmp(*this); ++(*this); return tmp; }
        const_iterator & operator++() {
            if (mp == nullptr) throw invalid_iterator();
            if (node == nullptr) throw invalid_iterator();
            node = node->list_next;
            return *this;
        }
        const_iterator operator--(int) { const_iterator tmp(*this); --(*this); return tmp; }
        const_iterator & operator--() {
            if (mp == nullptr) throw invalid_iterator();
            if (node == nullptr) { node = mp->tail; if (node == nullptr) throw invalid_iterator(); return *this; }
            if (node->list_prev == nullptr) throw invalid_iterator();
            node = node->list_prev;
            return *this;
        }
        reference operator*() const { if (node == nullptr) throw invalid_iterator(); return node->kv; }
        bool operator==(const const_iterator &rhs) const { return mp == rhs.mp && node == rhs.node; }
        bool operator==(const iterator &rhs) const { return mp == rhs.mp && node == rhs.node; }
        bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
        pointer operator->() const noexcept { return &(node->kv); }
    };
 
    linked_hashmap() : buckets(nullptr), bucket_cnt(0), elem_cnt(0), head(nullptr), tail(nullptr), hasher(Hash()), equal(Equal()) {
        reserve(8);
    }
    linked_hashmap(const linked_hashmap &other) : buckets(nullptr), bucket_cnt(0), elem_cnt(0), head(nullptr), tail(nullptr), hasher(other.hasher), equal(other.equal) {
        reserve(other.bucket_cnt > 8 ? other.bucket_cnt : 8);
        for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
            insert(*it);
        }
    }
 
    linked_hashmap & operator=(const linked_hashmap &other) {
        if (this == &other) return *this;
        clear();
        delete [] buckets; buckets = nullptr; bucket_cnt = 0;
        hasher = other.hasher; equal = other.equal;
        reserve(other.bucket_cnt > 8 ? other.bucket_cnt : 8);
        for (const_iterator it = other.cbegin(); it != other.cend(); ++it) insert(*it);
        return *this;
    }
 
    ~linked_hashmap() { clear(); if (buckets) { delete [] buckets; buckets = nullptr; } }
 
    T & at(const Key &key) {
        Node *n = find_node(key);
        if (!n) throw index_out_of_bound();
        return n->kv.second;
    }
    const T & at(const Key &key) const {
        Node *n = const_cast<linked_hashmap*>(this)->find_node(key);
        if (!n) throw index_out_of_bound();
        return n->kv.second;
    }
 
    T & operator[](const Key &key) {
        Node *n = find_node(key);
        if (n) return n->kv.second;
        if (elem_cnt + 1 > bucket_cnt * 3 / 4) rehash(bucket_cnt * 2 + 1);
        Node *nn = new Node(key);
        append_list(nn);
        insert_bucket(nn);
        ++elem_cnt;
        return nn->kv.second;
    }
 
    const T & operator[](const Key &key) const {
        Node *n = const_cast<linked_hashmap*>(this)->find_node(key);
        if (!n) throw index_out_of_bound();
        return n->kv.second;
    }
 
    iterator begin() { return iterator(this, head); }
    const_iterator cbegin() const { return const_iterator(this, head); }
 
    iterator end() { return iterator(this, nullptr); }
    const_iterator cend() const { return const_iterator(this, nullptr); }
 
    bool empty() const { return elem_cnt == 0; }
 
    size_t size() const { return elem_cnt; }
 
    void clear() {
        Node *cur = head;
        while (cur) {
            Node *nxt = cur->list_next;
            delete cur;
            cur = nxt;
        }
        head = tail = nullptr;
        if (buckets) {
            for (size_t i = 0; i < bucket_cnt; ++i) buckets[i] = nullptr;
        }
        elem_cnt = 0;
    }
 
    pair<iterator, bool> insert(const value_type &value) {
        Node *exist = find_node(value.first);
        if (exist) return pair<iterator, bool>(iterator(this, exist), false);
        if (elem_cnt + 1 > bucket_cnt * 3 / 4) rehash(bucket_cnt * 2 + 1);
        Node *nn = new Node(value);
        append_list(nn);
        insert_bucket(nn);
        ++elem_cnt;
        return pair<iterator, bool>(iterator(this, nn), true);
    }
 
    void erase(iterator pos) {
        if (pos.mp != this || pos.node == nullptr) throw invalid_iterator();
        Node *target = pos.node;
        size_t idx = bucket_index(target->kv.first);
        Node *cur = buckets[idx], *prev = nullptr;
        while (cur && cur != target) { prev = cur; cur = cur->hash_next; }
        if (cur != target) throw invalid_iterator();
        if (prev) prev->hash_next = cur->hash_next; else buckets[idx] = cur->hash_next;
        if (target->list_prev) target->list_prev->list_next = target->list_next; else head = target->list_next;
        if (target->list_next) target->list_next->list_prev = target->list_prev; else tail = target->list_prev;
        delete target;
        --elem_cnt;
    }
 
    size_t count(const Key &key) const { return const_cast<linked_hashmap*>(this)->find_node(key) ? 1 : 0; }
 
    iterator find(const Key &key) { Node *n = find_node(key); return iterator(this, n); }
    const_iterator find(const Key &key) const { Node *n = const_cast<linked_hashmap*>(this)->find_node(key); return const_iterator(this, n); }
private:
    Node **buckets;
    size_t bucket_cnt;
    size_t elem_cnt;
    Node *head;
    Node *tail;
    Hash hasher;
    Equal equal;
    void reserve(size_t n) {
        bucket_cnt = n;
        buckets = new Node*[bucket_cnt];
        for (size_t i = 0; i < bucket_cnt; ++i) buckets[i] = nullptr;
    }
    size_t bucket_index(const Key &k) const {
        return static_cast<size_t>(hasher(k)) % bucket_cnt;
    }
    Node* find_node(const Key &key) {
        if (bucket_cnt == 0) return nullptr;
        size_t idx = bucket_index(key);
        Node *cur = buckets[idx];
        while (cur) {
            if (equal(cur->kv.first, key)) return cur;
            cur = cur->hash_next;
        }
        return nullptr;
    }
    void append_list(Node *n) {
        n->list_prev = tail;
        n->list_next = nullptr;
        if (tail) tail->list_next = n; else head = n;
        tail = n;
    }
    void insert_bucket(Node *n) {
        size_t idx = bucket_index(n->kv.first);
        n->hash_next = buckets[idx];
        buckets[idx] = n;
    }
    void rehash(size_t new_cnt) {
        if (new_cnt < 8) new_cnt = 8;
        Node **new_buckets = new Node*[new_cnt];
        for (size_t i = 0; i < new_cnt; ++i) new_buckets[i] = nullptr;
        Node *cur = head;
        while (cur) {
            cur->hash_next = nullptr;
            cur = cur->list_next;
        }
        Node *old_head = head;
        Node **old_buckets = buckets; size_t old_cnt = bucket_cnt;
        buckets = new_buckets; bucket_cnt = new_cnt;
        (void)old_cnt;
        cur = old_head;
        while (cur) {
            size_t idx = bucket_index(cur->kv.first);
            cur->hash_next = buckets[idx];
            buckets[idx] = cur;
            cur = cur->list_next;
        }
        if (old_buckets) delete [] old_buckets;
    }
};

}

#endif
