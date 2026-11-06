#pragma once
#include "../xrEngine/VehicleGrass.h"
#include <map>
#include <mutex>
#include <vector>

// COP_ENGINE_PATCH: COP-ENG-170 - renderer never observes mutable game objects.
namespace VehicleGrass
{
class Registry
{
    std::mutex mutex;
    std::map<std::uintptr_t, std::vector<Sphere>> owners;
public:
    void Set(std::uintptr_t owner, const Sphere* data, std::uint32_t count)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!count) { owners.erase(owner); return; }
        auto& value = owners[owner];
        value.clear();
        for (std::uint32_t i = 0; i < count && i < MaxPerCar; ++i)
            if (Valid(data[i])) value.push_back(data[i]);
        if (value.empty()) owners.erase(owner);
    }
    std::uint32_t Copy(Sphere* output, std::uint32_t capacity, float x, float y, float z, float distance)
    {
        if (!output || !capacity || !std::isfinite(distance) || distance <= 0.f) return 0;
        std::lock_guard<std::mutex> lock(mutex);
        std::uint32_t count = 0;
        for (const auto& owner : owners)
            for (const auto& s : owner.second)
                if (Touches(s, x, y, z, distance) && count < capacity) output[count++] = s;
        return count;
    }
};
}
