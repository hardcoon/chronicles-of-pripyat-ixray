#pragma once
#include <cmath>
#include <cstdint>

// COP_ENGINE_PATCH: COP-ENG-170 - versioned POD snapshot; no engine-class ABI changes.
namespace VehicleGrass
{
constexpr std::uint32_t Capacity = 512;
constexpr std::uint32_t MaxPerCar = 8;
struct Sphere
{
    float x, y, z, radius, minimum;
    std::uint32_t keep;
};
struct Deformation
{
    float x = 0.f, z = 0.f, scale = 1.f, padding = 0.f;
};
static_assert(sizeof(Sphere) == 24 && sizeof(Deformation) == 16, "Vehicle grass snapshot layout");
using Snapshot = std::uint32_t (__cdecl *)(Sphere*, std::uint32_t, float, float, float, float);

// COP_ENGINE_PATCH: COP-ENG-171 - separate contact target from the visible response.
struct Settings
{
    float bend_time = .20f; // Time to reach 95% of a stationary target.
    float recovery_time = 1.f; // Seconds per full scale unit, matching donor +frame_delta.
    bool honor_scale_save = false;
};
struct Timing
{
    float dt, bend_alpha, recovery_step;
    bool honor_scale_save;
};

inline float Clamp(float value, float low, float high)
{
    return value < low ? low : value > high ? high : value;
}
inline bool Valid(const Sphere& s)
{
    return std::isfinite(s.x) && std::isfinite(s.y) && std::isfinite(s.z) &&
        std::isfinite(s.radius) && s.radius > .01f && s.radius <= 32.f &&
        std::isfinite(s.minimum) && s.minimum > .01f && s.minimum <= 1.f;
}
inline bool Touches(const Sphere& s, float x, float y, float z, float extra = 0.f)
{
    const float dx = x - s.x, dy = y - s.y, dz = z - s.z, r = s.radius + extra;
    return dx * dx + dy * dy + dz * dz < r * r;
}
inline Settings Sanitize(Settings settings)
{
    settings.bend_time = std::isfinite(settings.bend_time) ? Clamp(settings.bend_time, .01f, 10.f) : .20f;
    settings.recovery_time = std::isfinite(settings.recovery_time) ? Clamp(settings.recovery_time, .01f, 3600.f) : 1.f;
    return settings;
}
inline Timing Frame(const Settings& input, float dt)
{
    const Settings settings = Sanitize(input);
    dt = std::isfinite(dt) ? Clamp(dt, 0.f, .1f) : 0.f;
    return {dt, -std::expm1(-2.995732274f * dt / settings.bend_time),
        dt / settings.recovery_time, settings.honor_scale_save};
}
inline void Recover(Deformation& state, float step)
{
    const float previous = state.scale;
    state.scale = Clamp(previous + step, previous, 1.f);
    const float ratio = previous < 1.f ? (1.f - state.scale) / (1.f - previous) : 0.f;
    state.x *= ratio;
    state.z *= ratio;
}
inline void Update(Deformation& state, Deformation& target, bool& keep, float x, float y, float z,
    const Sphere* spheres, std::uint32_t count, const Timing& timing)
{
    if (timing.dt <= 0.f) return;
    if (!timing.honor_scale_save) keep = false;
    bool touched = false;
    for (std::uint32_t i = 0; i < count; ++i)
    {
        const Sphere& s = spheres[i];
        if (!Touches(s, x, y, z)) continue;
        touched = true;
        const float dx = x - s.x, dy = y - s.y, dz = z - s.z;
        const float falloff = Clamp((dx * dx + dy * dy + dz * dz) / (s.radius * s.radius), s.minimum, 1.f);
        // Donor falloff at reference 60 Hz; accumulated target is independent of response smoothing.
        const float next = Clamp(target.scale * std::pow(falloff, 120.f * timing.dt), s.minimum, 1.f);
        if (next < target.scale)
        {
            target.scale = next;
            const float horizontal = std::sqrt(dx * dx + dz * dz);
            // Preserve the root; choose a stable direction at the exact sphere centre.
            const float amount = std::sqrt(1.f - next * next);
            target.x = horizontal > .001f ? dx / horizontal * amount : amount;
            target.z = horizontal > .001f ? dz / horizontal * amount : 0.f;
            if (s.keep && timing.honor_scale_save) keep = true;
        }
    }
    if (!touched && !keep && target.scale < 1.f) Recover(target, timing.recovery_step);
    if (target.scale < state.scale)
    {
        state.scale += (target.scale - state.scale) * timing.bend_alpha;
        state.x += (target.x - state.x) * timing.bend_alpha;
        state.z += (target.z - state.z) * timing.bend_alpha;
        if (state.scale - target.scale < .00001f) state = target;
    }
    else if (target.scale > state.scale)
        Recover(state, Clamp(target.scale - state.scale, 0.f, timing.recovery_step));
}
}
