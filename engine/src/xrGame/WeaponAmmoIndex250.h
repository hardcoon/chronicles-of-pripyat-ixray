#pragma once

// COP_ENGINE_PATCH: COP-ENG-250 - repair externally restored ammo indices before lookup.
// The first configured type is deterministic and stays within the weapon's caliber.
inline bool RestoreWeaponAmmoIndex250(u8& index, size_t count, LPCSTR section, u16 id, LPCSTR context)
{
    if (index < count) return true;
    const unsigned previous = index;
    index = 0;
    if (count == 0)
    {
        Msg("! [AMMO250] weapon=%s id=%u context=%s old=%u no configured ammo types; cartridge skipped",
            section, unsigned(id), context, previous);
        return false;
    }
    Msg("! [AMMO250] weapon=%s id=%u context=%s old=%u new=0 count=%u",
        section, unsigned(id), context, previous, unsigned(count));
    return true;
}
