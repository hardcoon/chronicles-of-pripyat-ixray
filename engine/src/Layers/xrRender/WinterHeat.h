// COP_ENGINE_PATCH: COP-ENG-254 - independent, transactional Winter heat field.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <mutex>

namespace cop_winter_heat
{
constexpr unsigned capacity = 64;
constexpr unsigned header_index = capacity * 2;
constexpr unsigned vector_count = header_index + 1;
constexpr float protocol = 1024.0f;
constexpr std::uint32_t expiry_ms = 5000;
using Vector = std::array<float, 4>;
using Buffer = std::array<Vector, vector_count>;

class State
{
    Buffer pending{}, committed{};
    std::array<bool, capacity> written{};
    std::uint32_t touched = 0;
    bool armed = false;
    mutable std::mutex mutex;

public:
    bool set(unsigned index, const Vector& centre, const Vector& parameters)
    {
        if (index >= capacity)
            return false;
        bool valid = true;
        for (unsigned axis = 0; axis < 4; ++axis)
            valid = valid && std::isfinite(centre[axis]) && std::isfinite(parameters[axis]);
        for (unsigned axis = 0; axis < 3; ++axis)
            valid = valid && std::abs(centre[axis]) < 1000000.0f;
        valid = valid && centre[3] >= 0.1f && centre[3] <= 8.0f
            && parameters[0] >= 0.0f && parameters[0] <= 1.0f
            && std::abs(parameters[1]) <= 0.8f && std::abs(parameters[2]) <= 0.8f
            && parameters[3] == 0.0f;
        std::lock_guard<std::mutex> guard(mutex);
        written[index] = valid;
        if (!valid)
            return false;
        pending[index * 2] = centre;
        pending[index * 2 + 1] = parameters;
        return true;
    }

    bool commit(unsigned count, std::uint32_t now)
    {
        if (count > capacity)
            return false;
        std::lock_guard<std::mutex> guard(mutex);
        for (unsigned index = 0; index < count; ++index)
            if (!written[index])
                return false;
        committed = {};
        if (count)
        {
            std::copy_n(pending.begin(), count * 2, committed.begin());
            committed[header_index][3] = protocol + float(count);
        }
        pending = {};
        written = {};
        touched = now;
        armed = count != 0;
        return true;
    }

    void snapshot(Buffer& output, std::uint32_t now, bool paused)
    {
        std::lock_guard<std::mutex> guard(mutex);
        if (armed)
        {
            if (paused)
                touched = now;
            else if (std::uint32_t(now - touched) > expiry_ms)
            {
                committed = {};
                pending = {};
                written = {};
                armed = false;
            }
        }
        output = committed;
    }
};

extern State sources;
}
