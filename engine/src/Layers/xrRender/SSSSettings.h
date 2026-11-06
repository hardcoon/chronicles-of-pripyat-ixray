#pragma once

// COP_ENGINE_PATCH: COP-ENG-126 - private R4 addon parameters; never alias native graphics settings.
#ifdef USE_DX11
extern int ps_ssfx_enabled;
extern u32 ps_ssfx_revision;
extern u32 ps_ssfx_sun_frame,ps_ssfx_local_frame;
extern Fvector4 ps_ssfx_local_lights[8];
extern Fvector4 ps_ssfx_lut_override;
extern u32 ps_ssfx_emissive_frame;
extern Fvector4 ps_ssfx_modules; // directional shadows, local shadows, AO, reserved
extern Fvector4 ps_ssfx_shadow; // directional length, local length, intensity, detail
extern Fvector4 ps_ssfx_quality; // directional steps, local steps, HUD length, maximum distance
extern Fvector4 ps_ssfx_ao; // samples, radius, intensity, maximum distance
extern Fvector4 ps_ssfx_ao2; // minimum visibility, HUD intensity, temporal blend, bilateral radius
// COP_ENGINE_PATCH: COP-ENG-128 - all values remain independent of native preferences.
enum SSSOptionIndex {
#define SSS_OPTION(name, index, initial, low, high) SSS_##name = index,
#include "SSSOptions.inc"
#undef SSS_OPTION
    SSS_option_count
};
constexpr u32 SSS_vector_count = (SSS_option_count + 3) / 4;
extern Fvector4 ps_ssfx_options[SSS_vector_count];
inline float& ssfx_option(u32 index) { return (&ps_ssfx_options[index / 4].x)[index % 4]; }
// COP_ENGINE_PATCH: COP-ENG-138 - only rain puddles and their reflections remain.
// Legacy full-pipeline callers keep their stock lighting, AA, rain and materials.
inline bool ssfx_active() { return false; }
inline bool ssfx_effects_active() { return ps_ssfx_enabled != 0; }
inline bool ssfx_puddles_active() { return ssfx_effects_active() && ssfx_option(SSS_puddles_enabled)>.5f; }
extern Fvector ps_ssfx_context_weapon; // aim factor, active weapon, reload state
extern Fvector ps_ssfx_bender_positions[24], ps_ssfx_bender_metadata[24], ps_ssfx_bender_directions[8];
extern Fvector ps_ssfx_context_ui; // inventory, PDA, item animation
inline bool ssfx_hybrid_aa() {
    return ssfx_active() ? ssfx_option(SSS_taa_enabled)>.5f && ssfx_option(SSS_taa_hud_spatial)>.5f : false;
}
#endif
