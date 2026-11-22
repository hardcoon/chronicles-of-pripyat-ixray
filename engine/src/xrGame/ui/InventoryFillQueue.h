#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

// COP_ENGINE_PATCH: COP-ENG-253 - bounded, main-thread inventory preparation.
// Own only IDs and immutable sort keys; never retain game objects or UI cells.
namespace inventory_fill
{
struct Key
{
    int width = 0, height = 0, group = 1000, order = 0;
    // Match shared_str's native interned-address ordering, not a new alphabetic order.
    std::uintptr_t section = 0;
    std::uint16_t id = 0;
};

inline bool Before(const Key& a, const Key& b)
{
    if (a.width != b.width) return a.width > b.width;
    if (a.height != b.height) return a.height > b.height;
    if (a.group != b.group) return a.group < b.group;
    if (a.order != b.order) return a.order < b.order;
    if (a.section != b.section) return a.section > b.section;
    return a.id > b.id;
}

class Queue
{
    struct Later
    {
        bool operator()(const Key& a, const Key& b) const { return Before(b, a); }
    };
    std::vector<std::uint16_t> ids;
    std::vector<Key> heap;
    std::unordered_set<std::uint16_t> pending;
    std::size_t cursor = 0;

public:
    void Clear() { ids.clear(); heap.clear(); pending.clear(); cursor = 0; }
    void Add(std::uint16_t id) { if (pending.insert(id).second) ids.push_back(id); }
    void Cancel(std::uint16_t id) { pending.erase(id); }
    bool Busy() const { return cursor < ids.size() || !heap.empty(); }
    bool Preparing() const { return cursor < ids.size(); }
    bool Contains(std::uint16_t id) const { return pending.count(id) != 0; }

    // Exactly one key per call. The caller owns the frame/time budget.
    template<class Resolve> void PrepareOne(Resolve resolve)
    {
        if (!Preparing()) return;
        const auto id = ids[cursor++];
        if (!Contains(id)) return;
        Key key;
        if (!resolve(id, key)) { Cancel(id); return; }
        key.id = id;
        heap.push_back(std::move(key));
        std::push_heap(heap.begin(), heap.end(), Later{});
    }

    // Return at most one entry, including cancelled-entry cleanup.
    bool Pop(std::uint16_t& id)
    {
        if (Preparing() || heap.empty()) return false;
        std::pop_heap(heap.begin(), heap.end(), Later{});
        id = heap.back().id;
        heap.pop_back();
        return pending.erase(id) != 0;
    }
};
}
