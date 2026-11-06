////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_script3.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server objects for ALife simulator, script export, the third part
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pch_script.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer_script_macroses.h"
#include <cmath>

//#ifdef XRGAME_EXPORTS
//#	include "alife_smart_terrain_task.h"
//#endif

using namespace luabind;

#pragma optimize("s",on)


void set_yaw (CSE_ALifeObjectPhysic *obj, const float yaw)
{
	obj->o_Angle.y = yaw;
}


// COP_ENGINE_PATCH: COP-ENG-HF-003 - generic light setup before online spawn.
static bool set_light_properties(CSE_ALifeObjectHangingLamp* lamp, const Fcolor& tint,
    float range, float coneDegrees, LPCSTR bone, bool spot, bool shadows)
{
    if (!lamp || !bone || !*bone || !std::isfinite(range) || range <= 0.f || range > 1000.f ||
        !std::isfinite(coneDegrees) || coneDegrees <= 0.f || coneDegrees >= 180.f ||
        !std::isfinite(tint.r) || !std::isfinite(tint.g) || !std::isfinite(tint.b) || !std::isfinite(tint.a))
        return false;
    lamp->color = tint.get();
    lamp->brightness = 1.f;
    lamp->range = range;
    lamp->spot_cone_angle = deg2rad(coneDegrees);
    lamp->light_main_bone = bone;
    lamp->light_ambient_bone = bone;
    lamp->flags.set(CSE_ALifeObjectHangingLamp::flPhysic, FALSE);
    lamp->flags.set(CSE_ALifeObjectHangingLamp::flTypeSpot, spot);
    lamp->flags.set(CSE_ALifeObjectHangingLamp::flCastShadow, shadows);
    lamp->flags.set(CSE_ALifeObjectHangingLamp::flVolumetric, spot);
    return true;
}

void CSE_ALifeObjectHangingLamp::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife2(
			CSE_ALifeObjectHangingLamp,
			"cse_alife_object_hanging_lamp",
			CSE_ALifeDynamicObjectVisual,
			CSE_PHSkeleton
		)
		.def("set_light_properties", &set_light_properties)
	];
}
void CSE_ALifeObjectPhysic::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife2(
			CSE_ALifeObjectPhysic,
			"cse_alife_object_physic",
			CSE_ALifeDynamicObjectVisual,
			CSE_PHSkeleton
		)
		.def("set_yaw",				&set_yaw)
	];
}

void CSE_ALifeSmartZone::script_register(lua_State *L)
{
	module(L)[
		luabind_class_zone2(
			CSE_ALifeSmartZone,
			"cse_alife_smart_zone",
			CSE_ALifeSpaceRestrictor,
			CSE_ALifeSchedulable
		)
	];
}

