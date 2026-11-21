#pragma once

#include <cmath>

// COP_ENGINE_PATCH: COP-ENG-224 - Hike camera-driven weapon translation/HPB.
// Hike 2026-11-07: CHudItem::UpdateHudAdditional, RVA 0x433267..0x433c02.
// Keep the integration independent of rendering so the actual production math
// can be checked against the donor's machine code and at different frame rates.
namespace weapon_hike
{
struct Parameters
{
    float offset[4]; // left, right, up, down; metres
    float rotation[4]; // heading left/right, pitch up/down; degrees
    float bank[2]; // driven by yaw, despite the donor's "FB" name
    float response;
    float returnSpeed;
    float threshold; // camera angular velocity, rad/s
};

struct Settings
{
    Parameters hip{{-.008f, .008f, .005f, .005f}, {4.f, 4.f, -2.f, -2.f}, {6.f, 6.f}, .25f, .9f, .4f};
    Parameters aim{{.005f, .005f, .005f, .005f}, {2.5f, 2.5f, -1.5f, -1.5f}, {3.f, 3.f}, .25f, .95f, .4f};
    float transitionTime = .6f;
};

inline float bounded(float value, float low, float high)
{
    return value < low ? low : (value > high ? high : value);
}

inline float blend(float a, float b, float weight)
{
    return a + (b - a) * weight;
}

struct State
{
    float lr = 0.f, ud = 0.f, fb = 0.f;
    float position[3]{};
    float rotation[3]{}; // heading, pitch, bank; radians, subtracted from HUD HPB

    void reset() { *this = State{}; }

    static float settle(float value, float speed, float dt, bool stopped)
    {
        value = bounded(value, -1.f, 1.f) * bounded(1.f - dt * speed, 0.f, 1.f);
        if (stopped)
        {
            const float step = dt * speed * .075f;
            value = value < 0.f ? bounded(value + step, -1.f, 0.f) : bounded(value - step, 0.f, 1.f);
        }
        return value;
    }

    void update(float dt, float yaw, float pitch, float aim, const Settings& settings)
    {
        if (!std::isfinite(dt) || !std::isfinite(yaw) || !std::isfinite(pitch) ||
            !std::isfinite(aim) || dt < 0.f || dt > .25f)
        {
            reset();
            return;
        }
        if (dt == 0.f)
            return;

        aim = bounded(aim, 0.f, 1.f);
        const auto& h = settings.hip;
        const auto& a = settings.aim;
        const float response = blend(h.response, a.response, aim);
        const float returnSpeed = blend(h.returnSpeed, a.returnSpeed, aim);
        const float threshold = blend(h.threshold, a.threshold, aim);
        // The donor caches a frame delta. Use the current delta and bounded
        // substeps: identical 60 Hz response without FPS-dependent acceleration
        // or a negative interpolation weight during a long frame.
        const int steps = static_cast<int>(std::ceil(dt * 60.f));
        const float step = dt / static_cast<float>(steps);
        const float smoothing = bounded(step / settings.transitionTime, 0.f, 1.f);
        constexpr float radians = .01745329251994329577f;
        for (int i = 0; i < steps; ++i)
        {
            const bool moving = lr != 0.f || ud != 0.f || fb != 0.f;
            if (std::fabs(yaw) > threshold || moving)
            {
                lr -= yaw * step * response;
                fb -= yaw * step * response;
            }
            if (std::fabs(pitch) > threshold || moving)
            {
                const float reversal = pitch * ud > 0.f ? 2.f : 1.f;
                ud -= pitch * step * response * reversal;
            }
            lr = settle(lr, returnSpeed, step, yaw == 0.f);
            ud = settle(ud, returnSpeed, step, pitch == 0.f);
            fb = settle(fb, returnSpeed, step, yaw == 0.f);

            const int horizontal = lr < 0.f ? 0 : 1;
            const int vertical = ud < 0.f ? 2 : 3;
            const int roll = fb < 0.f ? 0 : 1;
            const float targetPosition[3] = {
                -blend(h.offset[horizontal], a.offset[horizontal], aim) * lr,
                blend(h.offset[vertical], a.offset[vertical], aim) * ud, 0.f};
            const float targetRotation[3] = {
                blend(h.rotation[horizontal], a.rotation[horizontal], aim) * lr * radians,
                -blend(h.rotation[vertical], a.rotation[vertical], aim) * ud * radians,
                -blend(h.bank[roll], a.bank[roll], aim) * fb * radians};
            for (int axis = 0; axis != 3; ++axis)
            {
                position[axis] = blend(position[axis], targetPosition[axis], smoothing);
                rotation[axis] = blend(rotation[axis], targetRotation[axis], smoothing);
                // Reach the authored sight pose exactly after the turn settles.
                if (targetPosition[axis] == 0.f && std::fabs(position[axis]) < 1.e-7f)
                    position[axis] = 0.f;
                if (targetRotation[axis] == 0.f && std::fabs(rotation[axis]) < 1.e-7f)
                    rotation[axis] = 0.f;
            }
        }
    }
};
}
