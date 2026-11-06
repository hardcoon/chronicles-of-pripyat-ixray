#include "StdAfx.h"
#include "UIOutfitInfo.h"
#include "../../xrUI/UIXmlInit.h"
#include "../../xrUI/Widgets/UIStatic.h"
#include "../../xrUI/Widgets/UIDoubleProgressBar.h"
#include "UIHelperGame.h"

#include "../CustomOutfit.h"
#include "../ActorHelmet.h"
#include "../Actor.h"
#include "../ActorCondition.h"
#include "../player_hud.h"
#include "../../xrEngine/string_table.h"

namespace
{
u32 condition_color_channel(u32 from, u32 to, float factor)
{
	return iFloor(float(from) + (float(to) - float(from)) * factor + 0.5f);
}

// COP_ENGINE_PATCH: COP-ENG-032 - expose outfit and helmet condition in their
// tooltips using the same red/yellow/green palette as the condition bar.
u32 condition_text_color(float condition)
{
	condition = clampr(condition, 0.0f, 1.0f);
	const bool upper_half = condition >= 0.5f;
	const float factor = upper_half ? (condition - 0.5f) * 2.0f : condition * 2.0f;
	const u32 red = condition_color_channel(upper_half ? 255 : 196, upper_half ? 107 : 255, factor);
	const u32 green = condition_color_channel(upper_half ? 255 : 18, upper_half ? 207 : 255, factor);
	const u32 blue = condition_color_channel(upper_half ? 118 : 18, upper_half ? 119 : 118, factor);
	return color_rgba(red, green, blue, 255);
}
}


LPCSTR immunity_names[]=
{
	"burn_immunity",
	"shock_immunity",
	"chemical_burn_immunity",
	"radiation_immunity",
	"telepatic_immunity",
	"wound_immunity",		
	"fire_wound_immunity",
	"strike_immunity",
	"explosion_immunity",
};

LPCSTR immunity_st_names[]=
{
	"ui_inv_outfit_burn_protection",
	"ui_inv_outfit_shock_protection",
	"ui_inv_outfit_chemical_burn_protection",
	"ui_inv_outfit_radiation_protection",
	"ui_inv_outfit_telepatic_protection",
	"ui_inv_outfit_wound_protection",
	"ui_inv_outfit_fire_wound_protection",
	"ui_inv_outfit_strike_protection",
	"ui_inv_outfit_explosion_protection",
};

CUIOutfitImmunity::CUIOutfitImmunity()
{
	AttachChild(&m_name);
	AttachChild(&m_progress);
	m_unit_str._set("");
	m_value = nullptr;
	m_magnitude = 1.0f;
}

CUIOutfitImmunity::~CUIOutfitImmunity()
{
}

bool CUIOutfitImmunity::InitFromXml( CUIXml& xml_doc, LPCSTR base_str, u32 hit_type )
{
	CUIXmlInit::InitWindow( xml_doc, base_str, 0, this );

	string256 buf;
	
	xr_strconcat(buf, base_str, ":", immunity_names[hit_type] );
	if (!CUIXmlInit::InitWindow( xml_doc, buf, 0, this, false ))
		return false;

	CUIXmlInit::InitStatic( xml_doc, buf, 0, &m_name );
	m_name.TextItemControl()->SetTextST( immunity_st_names[hit_type] );

	xr_strconcat(buf, base_str, ":", immunity_names[hit_type], ":progress_immunity" );
	m_progress.InitFromXml( xml_doc, buf );
	
	xr_strconcat(buf, base_str, ":", immunity_names[hit_type], ":static_value" );
	m_value = UIHelper::CreateTextWnd(xml_doc, buf, this);

	m_magnitude = xml_doc.ReadAttribFlt( buf, 0, "magnitude", 1.0f );

	LPCSTR unit_str = xml_doc.ReadAttrib(buf, 0, "unit_str", "");
	m_unit_str._set(g_pStringTable->translate(unit_str));
	return true;
}

void CUIOutfitImmunity::SetProgressValue(float cur, float comp)
{
	cur *= m_magnitude;
	comp *= m_magnitude;
	m_progress.SetTwoPos(cur, comp);

	string32 buf;
	xr_sprintf(buf, "%.0f", cur);

	string256 str;
	if (m_unit_str.size())
		xr_strconcat(str, buf, m_unit_str.c_str());
	else
		xr_strconcat(str, buf);

	m_value->SetText(str);
}

// ===========================================================================================

CUIOutfitInfo::CUIOutfitInfo()
{
	m_caption = nullptr;
	m_Prop_line = nullptr;
	m_iconCondition = nullptr;
	m_textCondition = nullptr;
	m_textConditionValue = nullptr;
	for ( u32 i = 0; i < max_count; ++i )
	{
		m_items[i] = nullptr;
	}
}

CUIOutfitInfo::~CUIOutfitInfo()
{
	for ( u32 i = 0; i < max_count; ++i )
	{
		xr_delete( m_items[i] );
	}
}

void CUIOutfitInfo::InitFromXml( CUIXml& xml_doc )
{
	LPCSTR base_str	= "outfit_info";

	CUIXmlInit::InitWindow( xml_doc, base_str, 0, this );
	
	string128 buf;

	xr_strconcat(buf, base_str, ":caption");
	if (xml_doc.NavigateToNode(buf))
	{
		m_caption = UIHelper::CreateStatic(xml_doc, buf, this);
	}

	xr_strconcat(buf, base_str, ":", "prop_line");
	if (xml_doc.NavigateToNode(buf))
	{
		m_Prop_line = UIHelper::CreateStatic(xml_doc, buf, this);
	}

	xr_strconcat(buf, base_str, ":static_condition");
	if (xml_doc.NavigateToNode(buf))
		m_iconCondition = UIHelper::CreateStatic(xml_doc, buf, this);
	xr_strconcat(buf, base_str, ":cap_condition");
	if (xml_doc.NavigateToNode(buf))
		m_textCondition = UIHelper::CreateTextWnd(xml_doc, buf, this);
	xr_strconcat(buf, base_str, ":cap_condition2");
	if (xml_doc.NavigateToNode(buf))
		m_textConditionValue = UIHelper::CreateTextWnd(xml_doc, buf, this);

	Fvector2 pos;
	if (m_Prop_line)
		pos.set(0.0f, m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y);
	else if (m_caption)
		pos.set(0.0f, m_caption->GetWndSize().y);
	else
		pos.set(0.0f, 0.0f);

	if (m_iconCondition)
		pos.y = _max(pos.y, m_iconCondition->GetWndPos().y + m_iconCondition->GetWndSize().y);
	if (m_textCondition)
		pos.y = _max(pos.y, m_textCondition->GetWndPos().y + m_textCondition->GetWndSize().y);
	if (m_textConditionValue)
		pos.y = _max(pos.y, m_textConditionValue->GetWndPos().y + m_textConditionValue->GetWndSize().y);

	for ( u32 i = 0; i < max_count; ++i )
	{
		m_items[i] = new CUIOutfitImmunity();
		if (m_items[i]->InitFromXml(xml_doc, base_str, i))
		{
			AttachChild(m_items[i]);
			m_items[i]->SetWndPos(pos);
			pos.y += m_items[i]->GetWndSize().y;
		}
		else
		{
			xr_delete(m_items[i]);
		}
	}
	pos.x = GetWndSize().x;
	SetWndSize( pos );
}

void CUIOutfitInfo::SetConditionValue(float condition)
{
	if (!m_textConditionValue)
		return;

	condition = clampr(condition, 0.0f, 1.0f);
	string16 value;
	xr_sprintf(value, "%d %%", iFloor(condition * 100.0f + 0.5f));
	m_textConditionValue->SetText(value);
	m_textConditionValue->SetTextColor(condition_text_color(condition));
}

void CUIOutfitInfo::UpdateInfo(CCustomOutfit* cur_outfit, CCustomOutfit* slot_outfit)
{
	CActor* actor = Level().CurrentViewEntity()->cast_actor();
	if ( !actor || !cur_outfit )
	{
		return;
	}
	SetConditionValue(cur_outfit->GetConditionToShow());

	for ( u32 i = 0; i < max_count; ++i )
	{
		if ( i == ALife::eHitTypeFireWound || !m_items[i] )
		{
			continue;
		}
		
		ALife::EHitType hit_type = (ALife::EHitType)i;
		float max_power = actor->conditions().GetZoneMaxPower( hit_type );

		float cur = cur_outfit->GetDefHitTypeProtection( hit_type );
		cur /= max_power; // = 0..1
		float slot = cur;
		
		if ( slot_outfit )
		{
			slot = slot_outfit->GetDefHitTypeProtection( hit_type );
			slot /= max_power; //  = 0..1
		}
		m_items[i]->SetProgressValue( cur, slot );
	}

	if ( m_items[ALife::eHitTypeFireWound] )
	{
		IKinematics* ikv = PKinematics(actor->Visual());
		VERIFY( ikv );
		u16 spine_bone = ikv->LL_BoneID( "bip01_spine" );

		float cur = cur_outfit->GetBoneArmor( spine_bone )*cur_outfit->GetCondition();
		//if(!cur_outfit->bIsHelmetAvaliable)
		//{
		//	spine_bone = ikv->LL_BoneID("bip01_head");
		//	cur += cur_outfit->GetBoneArmor(spine_bone);
		//}
		float slot = cur;
		if(slot_outfit)
		{
			spine_bone = ikv->LL_BoneID( "bip01_spine" );
			slot = slot_outfit->GetBoneArmor( spine_bone )*slot_outfit->GetCondition(); 
			//if(!slot_outfit->bIsHelmetAvaliable)
			//{
			//	spine_bone = ikv->LL_BoneID("bip01_head");
			//	slot += slot_outfit->GetBoneArmor(spine_bone);
			//}
		}
		float max_power = actor->conditions().GetMaxFireWoundProtection();
		cur /= max_power;
		slot /= max_power;
		m_items[ALife::eHitTypeFireWound]->SetProgressValue( cur, slot );
	}
}


void CUIOutfitInfo::UpdateInfo(CHelmet* cur_helmet, CHelmet* slot_helmet)
{
	CActor* actor = Level().CurrentViewEntity()->cast_actor();
	if ( !actor || !cur_helmet )
	{
		return;
	}
	SetConditionValue(cur_helmet->GetConditionToShow());

	for ( u32 i = 0; i < max_count; ++i )
	{
		if ( i == ALife::eHitTypeFireWound || !m_items[i] )
		{
			continue;
		}
		
		ALife::EHitType hit_type = (ALife::EHitType)i;
		float max_power = actor->conditions().GetZoneMaxPower( hit_type );

		float cur = cur_helmet->GetDefHitTypeProtection( hit_type );
		cur /= max_power; // = 0..1
		float slot = cur;
		
		if ( slot_helmet )
		{
			slot = slot_helmet->GetDefHitTypeProtection( hit_type );
			slot /= max_power; //  = 0..1
		}
		m_items[i]->SetProgressValue( cur, slot );
	}

	if ( m_items[ALife::eHitTypeFireWound] )
	{
		IKinematics* ikv = PKinematics(actor->Visual());
		VERIFY( ikv );
		u16 spine_bone = ikv->LL_BoneID( "bip01_head" );

		float cur = cur_helmet->GetBoneArmor( spine_bone )*cur_helmet->GetCondition();
		float slot = (slot_helmet)? slot_helmet->GetBoneArmor( spine_bone )*slot_helmet->GetCondition() : cur;
		// COP_ENGINE_PATCH: COP-ENG-029 - keep helmet and outfit armor on the
		// same configured fire-wound protection scale.
		const float max_power = actor->conditions().GetMaxFireWoundProtection();
		cur /= max_power;
		slot /= max_power;

		m_items[ALife::eHitTypeFireWound]->SetProgressValue( cur, slot );
	}

}
