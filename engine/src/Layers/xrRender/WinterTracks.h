// COP_ENGINE_PATCH: COP-ENG-234 - persistent native surface-track buffer.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>

namespace cop_winter
{
constexpr unsigned max_tracks = 128;
constexpr unsigned actor_index = max_tracks * 2;
constexpr unsigned tail_index = actor_index + 1;
constexpr unsigned vector_count = tail_index + 1;
constexpr float protocol = 256.0f;

using Vector = std::array<float, 4>;
using Buffer = std::array<Vector, vector_count>;

class TrackState
{
    std::array<Vector, actor_index> pending{};
    Buffer committed{};
    mutable std::mutex mutex;

    static bool valid_point(const Vector& value)
    {
        for (unsigned axis = 0; axis < 3; ++axis)
            if (!std::isfinite(value[axis]) || std::abs(value[axis]) > 1000000.0f)
                return false;
        return std::isfinite(value[3]);
    }

public:
    bool set(unsigned index, const Vector& start, const Vector& end, bool immediate)
    {
        if (index >= max_tracks || !valid_point(start) || !valid_point(end)
            || start[3] < 0.0f || start[3] > 31.0f || std::floor(start[3]) != start[3]
            || end[3] < 0.0f || end[3] > 1.0f)
            return false;
        std::lock_guard<std::mutex> guard(mutex);
        const float count = committed[tail_index][3] - protocol;
        if (immediate && (count < 1.0f || float(index) >= count))
            return false;
        pending[index * 2] = start;
        pending[index * 2 + 1] = end;
        if (immediate)
        {
            committed[index * 2] = start;
            committed[index * 2 + 1] = end;
        }
        return true;
    }

    bool live(const Vector& actor, const Vector& tail)
    {
        if (!valid_point(actor) || !valid_point(tail) || actor[3] < 0.0f || actor[3] > 1.0f)
            return false;
        std::lock_guard<std::mutex> guard(mutex);
        const float header = committed[tail_index][3];
        committed[actor_index] = actor;
        committed[tail_index] = tail;
        committed[tail_index][3] = header;
        return true;
    }

    bool commit(unsigned count)
    {
        if (count > max_tracks)
            return false;
        std::lock_guard<std::mutex> guard(mutex);
        if (count == 0)
        {
            committed = {};
            return true;
        }
        std::copy_n(pending.begin(), count * 2, committed.begin());
        std::fill(committed.begin() + count * 2, committed.begin() + actor_index, Vector{});
        committed[tail_index][3] = protocol + float(count);
        return true;
    }

    void snapshot(Buffer& output) const
    {
        std::lock_guard<std::mutex> guard(mutex);
        output = committed;
    }
};

extern TrackState tracks;
}