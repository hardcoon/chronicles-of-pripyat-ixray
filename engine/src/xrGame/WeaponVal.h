#pragma once

#include "WeaponMagazined.h"
#include "../xrScripts/script_export_space.h"

class CWeaponVal :
	public CWeaponMagazined
{
	typedef CWeaponMagazined inherited;
public:
	CWeaponVal(void);
	virtual ~CWeaponVal(void);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};