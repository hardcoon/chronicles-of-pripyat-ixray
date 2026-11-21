#pragma once

// COP_ENGINE_PATCH: COP-ENG-231 - time-based, interruptible HUD visibility.
struct SDynamicHudFade
{
    u32 last_activity = 0;
    float opacity = 1.0f;
    bool initialized = false;

    void Reset()
    {
        initialized = false;
        opacity = 1.0f;
    }

    void Touch(u32 now)
    {
        last_activity = now;
        initialized = true;
    }

    void Update(u32 now, float frame_time, bool active, float hold, float fade, float appear)
    {
        if (!initialized || active)
            Touch(now);

        const float idle = float(now - last_activity) * 0.001f;
        const float target = clampr(1.0f - (idle - hold) / fade, 0.0f, 1.0f);
        opacity = target > opacity ? _min(target, opacity + frame_time / appear) : target;
    }
};
