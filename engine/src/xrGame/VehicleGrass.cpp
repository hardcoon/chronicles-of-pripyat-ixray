#include "StdAfx.h"
#include "Car.h"
#include "VehicleGrassRegistry.h"

namespace VehicleGrass
{
static Registry& Cars()
{
    static Registry registry;
    return registry;
}
}

// COP_ENGINE_PATCH: COP-ENG-170 - optional, versioned bridge resolved by the renderer.
extern "C" __declspec(dllexport) std::uint32_t __cdecl xrVehicleGrassSnapshot170(
    VehicleGrass::Sphere* output, std::uint32_t capacity, float x, float y, float z, float distance)
{
    return VehicleGrass::Cars().Copy(output, capacity, x, y, z, distance);
}

void CCar::LoadGrassSpheres(CInifile* ini)
{
    ClearGrassSpheres();
    if (!ini || !ini->line_exist("car_definition", "grass_coll_spheres")) return;
    LPCSTR list = ini->r_string("car_definition", "grass_coll_spheres");
    const int count = _GetItemCount(list);
    for (int i = 0; i < count && i < int(VehicleGrass::MaxPerCar); ++i)
    {
        string128 section;
        _GetItem(list, i, section);
        if (!ini->section_exist(section) || !ini->line_exist(section, "position") || !ini->line_exist(section, "radius")) continue;
        const Fvector pos = ini->r_fvector3(section, "position");
        VehicleGrass::Sphere s = {pos.x, pos.y, pos.z, ini->r_float(section, "radius"),
            READ_IF_EXISTS(ini, r_float, section, "scale_factor", .25f),
            READ_IF_EXISTS(ini, r_bool, section, "scale_save", TRUE) ? 1u : 0u};
        if (VehicleGrass::Valid(s)) m_grass_spheres.push_back(s);
    }
    UpdateGrassSpheres();
}

void CCar::UpdateGrassSpheres()
{
    if (m_grass_spheres.empty()) return;
    VehicleGrass::Sphere world[VehicleGrass::MaxPerCar];
    for (u32 i = 0; i < m_grass_spheres.size(); ++i)
    {
        world[i] = m_grass_spheres[i];
        Fvector local, position;
        local.set(world[i].x, world[i].y, world[i].z);
        XFORM().transform_tiny(position, local);
        world[i].x = position.x; world[i].y = position.y; world[i].z = position.z;
    }
    VehicleGrass::Cars().Set(reinterpret_cast<std::uintptr_t>(this), world, u32(m_grass_spheres.size()));
}

void CCar::ClearGrassSpheres()
{
    VehicleGrass::Cars().Set(reinterpret_cast<std::uintptr_t>(this), nullptr, 0);
    m_grass_spheres.clear();
}
