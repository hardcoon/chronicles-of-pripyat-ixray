#pragma once
// COP_ENGINE_PATCH: COP-ENG-156
// Small pure distance helpers shared by Game and R4; no exported ABI or state.
inline bool PFInteriorInsideHorizon(float distance, float radius, float far_plane)
{
    return distance - radius <= _max(1.f, far_plane);
}
inline float PFInteriorDetailBudget(float far_plane, u32 profile, float visibility, float detail_scale)
{
    const float factor = profile == 0 ? detail_scale : _min(visibility, 0.5f);
    return _max(100.f, _max(1.f, far_plane - PFFarViewCameraExtension(profile)) * factor);
}
