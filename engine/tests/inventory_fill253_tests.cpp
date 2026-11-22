#include "../src/xrGame/ui/InventoryFillQueue.h"
#include <cassert>
#include <iostream>
#include <random>
#include <unordered_map>

// Native replay of the production queue, including cancellation between frames.
int main()
{
    using namespace inventory_fill;
    std::mt19937 rng(253);
    unsigned assertions = 0;
    auto check = [&](bool condition) { ++assertions; if (!condition) std::abort(); };
    for (unsigned count : {0u, 1u, 63u, 64u, 200u, 500u, 1000u, 10000u})
    {
        std::unordered_map<std::uint16_t, Key> objects;
        std::vector<Key> expected;
        Queue q;
        for (unsigned i = 0; i < count; ++i)
        {
            Key key;
            key.id = std::uint16_t(i);
            key.width = rng() % 5 + 1;
            key.height = rng() % 4 + 1;
            key.group = int(rng() % 12) - 2;
            key.order = int(rng() % 7) - 3;
            key.section = rng() % 20; // interned section order, deliberately unrelated to names
            objects[key.id] = key;
            expected.push_back(key);
            q.Add(key.id);
            q.Add(key.id); // duplicate snapshot entries must not duplicate cells
        }
        std::sort(expected.begin(), expected.end(), Before);
        unsigned prepared = 0;
        while (q.Preparing())
        {
            q.PrepareOne([&](auto id, Key& key) { ++prepared; key = objects.at(id); return true; });
            std::uint16_t id;
            if (q.Preparing()) check(!q.Pop(id));
        }
        check(prepared == count);
        unsigned emitted = 0;
        while (q.Busy())
        {
            std::uint16_t id;
            if (q.Pop(id)) { check(id == expected.at(emitted).id); ++emitted; }
        }
        check(emitted == count);
        check(!q.Busy());
        q.Clear();
    }
    Queue q;
    for (unsigned i = 0; i < 1000; ++i) q.Add(std::uint16_t(i));
    for (unsigned i = 0; i < 1000; i += 3) q.Cancel(std::uint16_t(i));
    while (q.Preparing()) q.PrepareOne([](auto id, Key& key) { key.id = id; key.width = id; return id % 5 != 0; });
    // Ownership changed after key preparation: old heap entries must be skipped.
    for (unsigned i = 1; i < 1000; i += 3) q.Cancel(std::uint16_t(i));
    std::unordered_set<unsigned> seen;
    while (q.Busy())
    {
        std::uint16_t id;
        if (q.Pop(id)) { check(id % 3 == 2 && id % 5 != 0); check(seen.insert(id).second); }
    }
    unsigned expected = 0;
    for (unsigned i = 0; i < 1000; ++i) if (i % 3 == 2 && i % 5 != 0) ++expected;
    check(seen.size() == expected);
    q.Add(42); q.Clear(); q.Add(42);
    q.PrepareOne([](auto, Key& key) { key.width = 5; return true; });
    std::uint16_t id;
    check(q.Pop(id) && id == 42 && !q.Busy());
    q.Add(7); q.PrepareOne([](auto, Key&) { return true; }); q.Clear();
    check(!q.Busy() && !q.Pop(id));
    std::cout << "PASS: " << assertions << " queue/order/lifecycle assertions\n";
}
