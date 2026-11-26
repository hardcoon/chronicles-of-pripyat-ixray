#pragma once

#include <cmath>

// COP_ENGINE_PATCH: COP-ENG-265 - an optional tail inside the object's sized STATE packet.
namespace vehicle_service
{
constexpr const char* profile_line = "vehicle_service_profile";
constexpr const char* canister_section = "pf_vehicle_fuel_canister";
constexpr float canister_capacity = 20.f;
constexpr float dry_drive_limit = 5.f;
constexpr u8 save_version = 1;

inline float bounded(float value, float maximum)
{
    return std::isfinite(value) ? clampr(value, 0.f, maximum) : 0.f;
}

inline u32 state_end(NET_Packet& packet, u16 size)
{
    if (packet.inistream || size < sizeof(u16))
        return 0;

    const u32 end = packet.r_tell() + size - sizeof(u16);
    return end <= packet.B.count ? end : 0;
}

struct State
{
    float coolant = 1.f;
    float battery = 1.f;
    float plugs = 1.f;
    float belt = 1.f;
    float dry_seconds = 0.f;
    bool initialized = false;

    void seed(bool active)
    {
        if (initialized)
            return;
        coolant = battery = plugs = belt = active ? 1.f : 0.f;
        initialized = true;
    }

    void write(NET_Packet& packet) const
    {
        if (!initialized || packet.inistream)
            return;
        packet.w_u8(save_version);
        packet.w_float(coolant);
        packet.w_float(battery);
        packet.w_float(plugs);
        packet.w_float(belt);
        packet.w_float(dry_seconds);
    }

    void read(NET_Packet& packet, u32 end)
    {
        *this = State{};
        constexpr u32 payload_size = sizeof(u8) + 5 * sizeof(float);
        if (!end || packet.r_tell() + payload_size != end || packet.r_u8() != save_version)
            return;
        coolant = bounded(packet.r_float(), 1.f);
        battery = bounded(packet.r_float(), 1.f);
        plugs = bounded(packet.r_float(), 1.f);
        belt = bounded(packet.r_float(), 1.f);
        dry_seconds = bounded(packet.r_float(), dry_drive_limit);
        initialized = true;
    }
};
}
