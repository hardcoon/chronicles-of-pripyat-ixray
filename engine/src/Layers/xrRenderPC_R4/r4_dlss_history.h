#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>

// COP_ENGINE_PATCH: COP-ENG-163 - history belongs to a contiguous scene/view.
struct DLSSFrameState
{
    std::uint32_t frame = 0, revision = 0, width = 0, height = 0;
    bool secondaryViewport = false;
    float position[3] = {}, direction[3] = {}, fov = 0.f;
};

class DLSSHistoryState
{
    bool valid = false;
    DLSSFrameState previous = {};
    // COP_ENGINE_PATCH: COP-ENG-188 - finite checks must survive the renderer's /fp:fast.
    static bool finite(float value)
    {
        std::uint32_t bits; std::memcpy(&bits, &value, sizeof(bits));
        return (bits & 0x7f800000u) != 0x7f800000u;
    }
public:
    void reset() { valid = false; }
    // COP_ENGINE_PATCH: COP-ENG-177 - a reset bit did not recover the captured black NGX instance.
    bool configurationChanged(const DLSSFrameState& now) const {
        return valid && (now.revision!=previous.revision || now.width!=previous.width || now.height!=previous.height);
    }
    // COP_ENGINE_PATCH: COP-ENG-188 - smooth zoom is reprojected by motion vectors.
    // Discarding accumulation for every 0.1-degree FOV step causes moving aliasing.
    // Keep scene/configuration/camera-cut protection and report the actual reason.
    const char* resetReason(const DLSSFrameState& now) const
    {
        if (!valid) return "new_history";
        if (!now.width || !now.height) return "invalid_size";
        if (now.frame - previous.frame != 1u) return "frame_gap";
        if (now.revision != previous.revision || now.width != previous.width || now.height != previous.height)
            return "configuration";
        if (now.secondaryViewport != previous.secondaryViewport) return "scope_transition";
        if (!finite(now.fov) || !finite(previous.fov)) return "invalid_camera";
        float distanceSquared = 0.f, directionDot = 0.f;
        for (unsigned i = 0; i < 3; ++i)
        {
            if (!finite(now.position[i]) || !finite(now.direction[i]) ||
                !finite(previous.position[i]) || !finite(previous.direction[i])) return "invalid_camera";
            const float delta = now.position[i] - previous.position[i];
            distanceSquared += delta * delta;
            directionDot += now.direction[i] * previous.direction[i];
        }
        if (distanceSquared > 16.f) return "teleport";
        if (directionDot < .8f) return "camera_cut";
        return nullptr;
    }
    bool needsReset(const DLSSFrameState& now) const { return resetReason(now) != nullptr; }
    void commit(const DLSSFrameState& now, bool success)
    {
        previous = now;
        valid = success;
    }
};
