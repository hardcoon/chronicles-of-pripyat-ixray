#include "stdafx.h"
#include "UICellItemFactory.h"
#include "UICellCustomItems.h"

CUICellItem* create_cell_item(CInventoryItem* itm)
{
	return create_cell_item(itm, false);
}

CUICellItem* create_cell_item(CInventoryItem* itm, bool deferWidgets)
{
	VERIFY(itm);
	CUICellItem* cell_item = nullptr;

	if (CWeaponAmmo* pAmmo = itm->cast_weapon_ammo())
	{
		cell_item = new CUIAmmoCellItem(pAmmo, deferWidgets);
	}
	else if (CWeapon* pWeapon = itm->cast_weapon())
	{
		cell_item = new CUIWeaponCellItem(pWeapon);
	}
	else
	{
		cell_item = new CUIInventoryCellItem(itm, deferWidgets);
	}

	return cell_item;
}
