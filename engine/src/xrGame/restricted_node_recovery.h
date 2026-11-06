#pragma once

#include <chrono>
#include <mutex>

// COP_ENGINE_PATCH: COP-ENG-186 - bounded recovery only; never changes Search or its access mask.
namespace cop_nav_recovery
{
using Clock = std::chrono::steady_clock;
constexpr unsigned query_nodes = 128;
constexpr unsigned frame_nodes = 256;
constexpr float radius_squared = 36.f;
constexpr auto frame_time = std::chrono::microseconds(250);
constexpr auto retry_time = std::chrono::milliseconds(500);

struct Failure
{
    const void* graph = nullptr;
    const void* restriction = nullptr;
    unsigned start = 0;
    bool inward = false;
    Clock::time_point until{};
};

struct Budget
{
    std::mutex mutex;
    unsigned frame = ~0u;
    unsigned nodes = 0;
    unsigned cursor = 0;
    unsigned requests = 0, attempts = 0, admission = 0;
    bool exhausted = false;
    Clock::duration spent{};
    Failure failures[32]{};
};

inline Budget& budget()
{
    static Budget state;
    return state;
}

inline void reset()
{
    auto& state = budget();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.frame = ~0u;
    state.nodes = state.cursor = 0;
    state.requests = state.attempts = state.admission = 0;
    state.exhausted = false;
    state.spent = Clock::duration::zero();
    for (auto& failure : state.failures) failure = Failure{};
}

template <class Graph, class Position, class Accept>
unsigned find(const Graph& graph, unsigned frame, unsigned start, const Position& source,
    const void* restriction, bool inward, Position& result, Accept accept)
{
    if (!graph.valid_vertex_id(start)) return ~0u;
    auto& state = budget();
    std::lock_guard<std::mutex> lock(state.mutex);
    const auto now = Clock::now();
    const void* identity = &graph.header();
    for (const auto& failure : state.failures)
        if (failure.graph == identity && failure.restriction == restriction
            && failure.start == start && failure.inward == inward && now < failure.until)
            return ~0u;
    if (state.frame != frame)
    {
        // Rotate admission only after contention: permanent failures must not
        // consume every frame before later NPCs get their recovery attempt.
        state.admission = state.exhausted && state.requests
            ? (state.admission + (state.attempts ? state.attempts : 1)) % state.requests : 0;
        state.frame = frame;
        state.nodes = 0;
        state.spent = Clock::duration::zero();
        state.requests = state.attempts = 0;
        state.exhausted = false;
    }
    if (state.requests++ < state.admission) return ~0u;
    if (state.nodes >= frame_nodes || state.spent >= frame_time)
    {
        state.exhausted = true;
        return ~0u;
    }
    ++state.attempts;

    // Fixed storage and real mesh links: no world scan, allocation, floor jump or mask edits.
    unsigned queue[query_nodes];
    unsigned head = 0, tail = 1;
    queue[0] = start;
    unsigned selected = ~0u;
    while (head < tail && state.nodes < frame_nodes)
    {
        if (state.spent + (Clock::now() - now) >= frame_time) break;
        const unsigned id = queue[head++];
        ++state.nodes;
        const auto point = graph.vertex_position(id);
        if (source.distance_to_sqr(point) > radius_squared) continue;
        if (graph.is_accessible(id) && accept(id, point))
        {
            result = point;
            selected = id;
            break;
        }
        for (unsigned edge = 0; edge < 4 && tail < query_nodes; ++edge)
        {
            const unsigned next = graph.vertex(id)->link(edge);
            if (!graph.valid_vertex_id(next)) continue;
            bool seen = false;
            for (unsigned i = 0; i < tail; ++i)
                if (queue[i] == next) { seen = true; break; }
            if (!seen) queue[tail++] = next;
        }
    }
    state.spent += Clock::now() - now;
    if (state.nodes >= frame_nodes || state.spent >= frame_time) state.exhausted = true;
    if (selected == ~0u)
        state.failures[state.cursor++ % 32] = {identity, restriction, start, inward, now + retry_time};
    return selected;
}
}
