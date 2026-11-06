#pragma once

#include "GameObject.h"
#include "../xrCore/xr_ini.h"

// COP_ENGINE_PATCH: COP-ENG-HF-010 - opt-in damage protection for physical objects.
// Keep the collision model, physics joints and explicit removal intact. No saved
// fields or class layout changes; existing saves pick up the current config.
inline bool IsObjectDamageImmune(const CGameObject& object)
{
    const auto& section = object.cNameSect();
    return pSettings && section.size() &&
        READ_IF_EXISTS(pSettings, r_bool, section.c_str(), "damage_immune", false);
}
