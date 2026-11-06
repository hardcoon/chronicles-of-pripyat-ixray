#include "StdAfx.h"
#include "pch_script.h"
#include "alife_space.h"
#include "Car.h"
#include "CarWeapon.h"
#include "script_game_object.h"

using namespace luabind;

#pragma optimize("s",on)
void CCar::script_register(lua_State *L)
{
	module(L)
	[
		class_<CCar,bases<CGameObject,CHolderCustom> >("CCar")
			.enum_("wpn_action")
				[
					value("eWpnDesiredDir",							int(CCarWeapon::eWpnDesiredDir)),
					value("eWpnDesiredPos",							int(CCarWeapon::eWpnDesiredPos)),
					value("eWpnActivate",							int(CCarWeapon::eWpnActivate)),
					value("eWpnFire",								int(CCarWeapon::eWpnFire)),
					value("eWpnAutoFire",							int(CCarWeapon::eWpnAutoFire)),
					value("eWpnToDefaultDir",						int(CCarWeapon::eWpnToDefaultDir))
				]
		.def("Action",			&CCar::Action)
//		.def("SetParam",		(void (CCar::*)(int,Fvector2)) &CCar::SetParam)
		.def("SetParam",		(void (CCar::*)(int,Fvector)) &CCar::SetParam)
		.def("CanHit",			&CCar::WpnCanHit)
		.def("FireDirDiff",		&CCar::FireDirDiff)
		.def("IsObjectVisible",	&CCar::isObjectVisible)
		.def("HasWeapon",		&CCar::HasWeapon)
		.def("weapon_mode_active", &CCar::IsWeaponMode)
		// COP_ENGINE_PATCH: COP-ENG-206 - no ammunition mutation exposed to scripts.
		.def("weapon_inventory_ammo", &CCar::WeaponInventoryAmmo)
		// COP_ENGINE_PATCH: COP-ENG-205 - read-only operator context for native QA/scripts.
		.def("weapon_external_view_active", &CCar::IsWeaponExternalView)
		.def("CurrentVel",		&CCar::CurrentVel)
		// COP_ENGINE_PATCH: COP-ENG-197 - native door interaction for scripts/QA.
		.def("switch_door", &CCar::DoorUse)
		// COP_ENGINE_PATCH: COP-ENG-094 - GPS reads ignition without polling physics or inventory.
		.def_readonly("engine_running", &CCar::b_engine_on)
		.def("GetfHealth",		&CCar::GetfHealth)
		.def("SetfHealth",		&CCar::SetfHealth)
		.def("SetExplodeTime",	&CCar::SetExplodeTime)
		.def("ExplodeTime",		&CCar::ExplodeTime)
		.def("CarExplode",		&CCar::CarExplode)
			
		// FX: [AIW]
		.def("add_fuel",			&CCar::AddFuel)
		// COP_ENGINE_PATCH: COP-ENG-061 - safe, profile-gated canister API.
		.def("fuel_system_enabled",	&CCar::FuelSystemEnabled)
		.def("fuel_refuel_distance",	&CCar::FuelRefuelDistance)
		.def("can_refuel_canister",	&CCar::CanRefuelCanister)
		.def("try_refuel_canister",	&CCar::TryRefuelCanister)
		.def_readwrite("fuel",		&CCar::m_fuel)
		.def_readwrite("fuel_tank",	&CCar::m_fuel_tank)

		.def(constructor<>())
	];
}
