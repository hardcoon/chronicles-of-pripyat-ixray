#include "../src/xrGame/PdaMapFilters.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main()
{
    using namespace PdaMapFilters;
    unsigned cases = 0;
    const Mask newSlots = All & ~LegacyMask;
    for (Mask original = 0; original <= LegacyMask; ++original)
    {
        const Mask mask = original | newSlots;
        std::uint8_t bytes[4];
        Encode(mask, bytes);
        assert(sizeof(bytes) == 4 && Decode(bytes) == mask);
        const std::uint8_t eng260[4] = {static_cast<std::uint8_t>(128 | original),
            static_cast<std::uint8_t>(Enabled(original, Category::Important)),
            static_cast<std::uint8_t>(Enabled(original, Category::Tasks)),
            static_cast<std::uint8_t>(Enabled(original, Category::Anomalies))};
        for (unsigned i = 0; i < 4; ++i) assert(bytes[i] == eng260[i]);
        for (int slot = 0; slot < SlotCount; ++slot)
        {
            const auto category = FromSlot(slot);
            const auto off = With(mask, category, false);
            const auto on = With(off, category, true);
            assert(!Enabled(off, category) && Enabled(on, category));
            for (int otherSlot = 0; otherSlot < SlotCount; ++otherSlot)
                if (otherSlot != slot)
                {
                    const auto other = FromSlot(otherSlot);
                    assert(Enabled(off, other) == Enabled(mask, other));
                    assert(Enabled(on, other) == Enabled(mask, other));
                }
            assert(!Visible(off, All, category) && Visible(off, With(All, category, false), category));
            Encode(off, bytes);
            assert(Decode(bytes) == off);
            ++cases;
        }
    }
    for (Mask legacy = 0; legacy < 16; ++legacy)
    {
        const std::uint8_t bytes[4] = {std::uint8_t(legacy & 1), std::uint8_t((legacy >> 1) & 1),
            std::uint8_t((legacy >> 2) & 1), std::uint8_t((legacy >> 3) & 1)};
        const auto restored = Decode(bytes);
        assert(Enabled(restored, Category::CommonStashes) == bool(legacy & 1));
        assert(Enabled(restored, Category::RareStashes) == bool(legacy & 1));
        assert(Enabled(restored, Category::Important) == bool(legacy & 2));
        assert(Enabled(restored, Category::Tasks) == bool(legacy & 4));
        assert(Enabled(restored, Category::Anomalies) == bool(legacy & 8));
        assert(Enabled(restored, Category::RadioBeacons) == bool(legacy & 8));
        assert((restored & newSlots) == newSlots);
        assert(Enabled(restored, Category::UserMarkers));
        ++cases;
    }

    for (Mask mask : {Mask{0}, All, Mask{0x0a534963}, Mask{0x07401102}})
    {
        std::uint8_t bytes[4];
        Encode(mask, bytes);
        assert(Decode(bytes) == mask);
        ++cases;
    }
    assert(FromSlot(-1) == Category::None && FromSlot(SlotCount) == Category::None);
    assert(FromSlot(6) == Category::UserMarkers);
    assert(Enabled(0, Category::None));
    assert(EffectiveCategory(Category::RadioBeacons, true) == Category::UserMarkers);
    assert(EffectiveCategory(FromSlot(9), false) == FromSlot(9));
    assert(!NeedsLegacySpotReset(0, "storyline_task_location", false));
    assert(!NeedsLegacySpotReset(0, "actor_location", false));
    assert(!NeedsLegacySpotReset(0, "pf_vehicle_gps_location", false));
    assert(NeedsLegacySpotReset(0, "treasure", false));
    assert(NeedsLegacySpotReset(0, "lz_nm_beacon_location", false));
    assert(NeedsLegacySpotReset(0, "secondary_task_location", false));
    assert(NeedsLegacySpotReset(0, "pf_user_marker_anomaly", true));
    assert(!NeedsLegacySpotReset(IndependentSpotFlags, "secondary_task_location", false));
    assert(!NeedsLegacySpotReset(IndependentSpotFlags, "pf_user_marker_anomaly", true));
    std::printf("PASS: %u C++ save and visibility cases across %d configurable slots\n", cases, SlotCount);
}
