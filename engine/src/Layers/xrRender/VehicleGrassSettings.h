#pragma once
#include "../../xrEngine/VehicleGrass.h"

// COP_ENGINE_PATCH: COP-ENG-171 - dedicated VFS config, read once when the level's details load.
inline VehicleGrass::Settings ReadVehicleGrassSettings(CInifile& ini)
{
    VehicleGrass::Settings settings;
    settings.bend_time = READ_IF_EXISTS(&ini, r_float, "vehicle_grass", "bend_time", settings.bend_time);
    settings.recovery_time = READ_IF_EXISTS(&ini, r_float, "vehicle_grass", "recovery_time", settings.recovery_time);
    settings.honor_scale_save = READ_IF_EXISTS(&ini, r_bool, "vehicle_grass", "honor_scale_save", settings.honor_scale_save);
    return VehicleGrass::Sanitize(settings);
}
