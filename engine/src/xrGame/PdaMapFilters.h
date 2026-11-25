#pragma once
#include <cstdint>
#include <cstring>

// COP_ENGINE_PATCH: COP-ENG-260 - native PDA filter categories and legacy save codec.
// This header has no engine dependencies so the actual policy can be tested natively.
namespace PdaMapFilters
{
using Mask = std::uint32_t;
enum class Category : Mask
{
    None = 0, CommonStashes = 1, RareStashes = 2, Anomalies = 4,
    RadioBeacons = 8, Tasks = 16, Important = 32, UserMarkers = 64
};
// COP_ENGINE_PATCH: COP-ENG-263 - stable XML slots share the existing four
// save bytes. Slots 0-6 retain their meanings in saves from eng-260 onward.
constexpr int SlotCount = 28;
constexpr Mask LegacyMask = 127;
constexpr Mask All = (Mask{1} << SlotCount) - 1;
// COP_ENGINE_PATCH: COP-ENG-262 - existing location record byte now identifies
// saves whose per-spot enable flag is independent of the PDA category.
constexpr std::uint8_t IndependentSpotFlags = 1;

constexpr Category FromSlot(int slot)
{
    return slot >= 0 && slot < SlotCount ? static_cast<Category>(Mask{1} << slot) : Category::None;
}
inline bool Enabled(Mask mask, Category category)
{
    return category == Category::None || (mask & static_cast<Mask>(category)) != 0;
}
inline Mask With(Mask mask, Category category, bool enabled)
{
    const auto bit = static_cast<Mask>(category);
    return (enabled ? mask | bit : mask & ~bit) & All;
}
inline bool Visible(Mask mask, Mask controls, Category category)
{
    return Enabled(mask | ~controls, category);
}
inline bool Is(const char* type, const char* expected)
{
    return type && std::strcmp(type, expected) == 0;
}
inline Category EffectiveCategory(Category category, bool userDefined)
{
    return userDefined ? Category::UserMarkers : category;
}
// Old UI and Lua filters overwrote the actual enable flag. Clear their stale
// state once on load, preserving individual hides written by the new engine.
inline bool NeedsLegacySpotReset(std::uint8_t recordVersion, const char* type, bool userDefined)
{
    if (recordVersion != 0) return false;
    // COP_ENGINE_PATCH: COP-ENG-004 - these beacons were once toggled by Locations.
    return userDefined || Is(type, "treasure") || Is(type, "primary_object") ||
        Is(type, "lz_nm_beacon_location") || Is(type, "lz_nm_beacon_focus_location") ||
        Is(type, "ui_pda2_trader_location") || Is(type, "ui_pda2_mechanic_location") ||
        Is(type, "ui_pda2_scout_location") || Is(type, "ui_pda2_quest_npc_location") ||
        Is(type, "ui_pda2_medic_location") || Is(type, "ui_pda2_actor_box_location") ||
        Is(type, "ui_pda2_actor_sleep_location") ||
        Is(type, "secondary_task_location") || Is(type, "secondary_task_location_complex_timer");
}
inline void Encode(Mask mask, std::uint8_t (&bytes)[4])
{
    // Keep exactly four actor bytes: the following camera/quickslot data stays aligned.
    bytes[0] = static_cast<std::uint8_t>(128 | (mask & LegacyMask));
    // Extra slots use the unused high seven bits of each compatibility byte.
    // Zero means enabled so old saves show any subsequently added group.
    const Mask disabled = All & ~mask;
    bytes[1] = static_cast<std::uint8_t>(((disabled >> 7) & LegacyMask) << 1 |
        Enabled(mask, Category::Important));
    bytes[2] = static_cast<std::uint8_t>(((disabled >> 14) & LegacyMask) << 1 |
        Enabled(mask, Category::Tasks));
    bytes[3] = static_cast<std::uint8_t>(((disabled >> 21) & LegacyMask) << 1 |
        Enabled(mask, Category::Anomalies));
}
inline Mask Decode(const std::uint8_t (&bytes)[4])
{
    if (bytes[0] & 128)
    {
        const Mask disabled = (static_cast<Mask>(bytes[1] >> 1u) << 7) |
            (static_cast<Mask>(bytes[2] >> 1u) << 14) | (static_cast<Mask>(bytes[3] >> 1u) << 21);
        return (bytes[0] & LegacyMask) | (All & ~LegacyMask & ~disabled);
    }
    auto mask = All;
    mask = With(mask, Category::CommonStashes, bytes[0] != 0);
    mask = With(mask, Category::RareStashes, bytes[0] != 0);
    mask = With(mask, Category::Important, bytes[1] != 0);
    mask = With(mask, Category::Tasks, bytes[2] != 0);
    mask = With(mask, Category::Anomalies, bytes[3] != 0);
    mask = With(mask, Category::RadioBeacons, bytes[3] != 0);
    return mask;
}
}
