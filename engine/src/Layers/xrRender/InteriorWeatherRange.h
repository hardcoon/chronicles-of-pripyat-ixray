#pragma once
#include "../../xrEngine/IRenderable.h"
#include "../../xrEngine/IGame_Persistent.h"
#include "../../xrEngine/Environment.h"
#include "../../xrEngine/InteriorRenderRange.h"
#include "xrRender_console.h"
// COP_ENGINE_PATCH: COP-ENG-156 - furniture remains scoped to its spatial tag.
inline bool PFInteriorWeatherRangeEnabled()
{
#if RENDER == R_R4
    return ps_r4_interior_weather_range && psDeviceFlags.test(rsR4);
#else
    return false;
#endif
}
inline bool PFFurnitureWeatherRange(IRenderable* object)
{
    return PFInteriorWeatherRangeEnabled() && object && object->SpatialComponent &&
        (object->SpatialComponent->spatial.type & STYPE_FURNITURE);
}
inline bool PFInteriorWeatherSpatial(const ISpatial* spatial)
{
    return PFInteriorWeatherRangeEnabled() && spatial &&
        (spatial->spatial.type & (STYPE_FURNITURE | STYPE_LIGHTSOURCE));
}
inline bool PFInteriorInsideWeather(const Fsphere& sphere)
{
    return g_pGamePersistent && g_pGamePersistent->Environment().CurrentEnv &&
        PFInteriorInsideHorizon(Device.vCameraPosition.distance_to(sphere.P), sphere.R,
            g_pGamePersistent->Environment().CurrentEnv->far_plane);
}
