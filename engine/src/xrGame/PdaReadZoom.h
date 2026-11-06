#pragma once
#include <algorithm>
#include <cmath>

// COP_ENGINE_PATCH: COP-ENG-214 - reading zoom is independent of draw/holster poses.
struct PdaReadZoom
{
    bool reading = false;
    float factor = 1.f;

    void reset() { reading = false; factor = 1.f; }
    void update(float dt, bool canRead, float requested)
    {
        if (!canRead) reading = false;
        // COP_ENGINE_PATCH: COP-ENG-215 - user-selected reading range 1..2.
        // COP_ENGINE_PATCH: COP-ENG-217 - requested default/fallback 1.7.
        const float limit = std::isfinite(requested) ? std::clamp(requested, 1.f, 2.f) : 1.7f;
        const float target = reading ? limit : 1.f;
        const float blend = 1.f - std::exp(-12.f * std::clamp(dt, 0.f, 1.f));
        factor += (target - factor) * blend;
        if (std::fabs(target - factor) < .0001f) factor = target;
    }
    float fov(float base) const
    {
        if (factor == 1.f) return base;
        constexpr float radians = 0.017453292519943295f;
        return 2.f * std::atan(std::tan(std::clamp(base, 5.f, 179.f) * radians * .5f) / factor) / radians;
    }
};
