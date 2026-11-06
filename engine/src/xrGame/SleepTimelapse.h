#pragma once
// COP_ENGINE_PATCH: COP-ENG-185 - transient camera ownership; no save-game fields.
namespace cop_sleep
{
    bool active();
    bool set(const Fvector& position, const Fvector& direction);
    void clear();
    float needs_factor();
}
