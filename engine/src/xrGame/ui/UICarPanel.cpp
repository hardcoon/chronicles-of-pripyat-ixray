#include "stdafx.h"
#include "UIMainIngameWnd.h"
#include "UICarPanel.h"
#include "../../xrUI/UIXmlInit.h"
#include "../../xrUI/UIHelper.h"
#include "../../xrEngine/string_table.h"

constexpr LPCSTR CAR_PANEL_XML = "car_panel.xml";
constexpr LPCSTR POINTER_ARROW_TEX = "ui\\hud_map_arrow";

void CUICarPanel::Init()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, CAR_PANEL_XML);

    CUIXmlInit xml_init;
    xml_init.InitWindow(uiXml, "car_panel", 0, this);
    ////////////////////////////////////////////////////////////////////
    UIStaticCarHealth = UIHelper::CreateStatic(uiXml, "car_panel:car_static", this);
    UIStaticCarHealth->SetAutoDelete(true);

    UIEngineSpeedMode = UIHelper::CreateTextWnd(uiXml, "car_panel:car_speed_mode", UIStaticCarHealth);
    UIEngineSpeedMode->SetAutoDelete(true);

    UICarFuelText = UIHelper::CreateTextWnd(uiXml, "car_panel:car_fuel_text", UIStaticCarHealth);
    UICarFuelText->SetAutoDelete(true);
    UICarFuelText->Show(false);
    // COP_ENGINE_PATCH: COP-ENG-206 - clone the configured text style in every aspect ratio.
    UIWeaponAmmo = UIHelper::CreateTextWnd(uiXml, "car_panel:car_fuel_text", UIStaticCarHealth);
    UIWeaponAmmo->SetAutoDelete(true);
    UIWeaponAmmo->SetWndPos(Fvector2().set(UIStaticCarHealth->GetWidth() - 360.f, -26.f));
    UIWeaponAmmo->SetWndSize(Fvector2().set(360.f, 24.f));
    UIWeaponAmmo->SetTextAlignment(CGameFont::alRight);
    UIWeaponAmmo->Show(false);
    UIFuelUnit = g_pStringTable->translate("pf_vehicle_hud_fuel_unit").c_str();

    UIEngineLamp = UIHelper::CreateStatic(uiXml, "car_panel:car_engine_lamp:on", UIStaticCarHealth);
    UIEngineLamp->SetAutoDelete(true);
    
    UIEngineLampOff = UIHelper::CreateStatic(uiXml, "car_panel:car_engine_lamp:off", UIStaticCarHealth);
    UIEngineLampOff->SetAutoDelete(true);

    UILightLamp = UIHelper::CreateStatic(uiXml, "car_panel:car_light_lamp:on", UIStaticCarHealth);
    UILightLamp->SetAutoDelete(true);
    
    UILightLampOff = UIHelper::CreateStatic(uiXml, "car_panel:car_light_lamp:off", UIStaticCarHealth);
    UILightLampOff->SetAutoDelete(true);

	UIConditionIcon = UIHelper::CreateStatic(uiXml, "car_panel:car_condition_icon", UIStaticCarHealth);
	UIConditionIcon->SetAutoDelete(true);

    UICarHealthBar = UIHelper::CreateProgressBar(uiXml, "car_panel:car_health_progress_bar", UIStaticCarHealth);
    UICarHealthBar->SetAutoDelete(true);
    
    UICarFuelBar = UIHelper::CreateProgressBar(uiXml, "car_panel:car_fuel_progress_bar", UIStaticCarHealth);
    UICarFuelBar->SetAutoDelete(true);

    Show(false);
}

void CUICarPanel::SetEngineLamp(bool On)
{
    if (On)
    {
        UIEngineLamp->Show(true);
        UIEngineLampOff->Show(false);
    }
    else
    {
        UIEngineLamp->Show(false);
        UIEngineLampOff->Show(true);
    }
}

void CUICarPanel::SetLightLamp(bool On)
{
    if (On)
    {
        UILightLamp->Show(true);
        UILightLampOff->Show(false);
    }
    else
    {
        UILightLamp->Show(false);
        UILightLampOff->Show(true);
    }
}

void CUICarPanel::Draw()
{
    inherited::Draw();
}

//////////////////////////////////////////////////////////////////////////

CUICarPanel::~CUICarPanel()
{
}

void CUICarPanel::SetCarHealth(float value, float warning_threshold, float critical_threshold)
{
	// COP_ENGINE_PATCH: COP-ENG-068 - the condition gear is no longer a white
	// bitmap baked into the panel. Its severity is driven by the same critical
	// threshold that starts profile damage smoke.
	const float health = clampr(value, 0.f, 1.f);
	UICarHealthBar->SetProgressPos(health * 100.f);
	if (health <= critical_threshold)
		UIConditionIcon->SetTextureColor(color_rgba(154, 54, 44, 255));
	else if (health <= warning_threshold)
		UIConditionIcon->SetTextureColor(color_rgba(181, 133, 0, 255));
	else
		UIConditionIcon->SetTextureColor(color_rgba(194, 194, 185, 255));
}

void CUICarPanel::SetCarFuel(float value, float capacity, bool enabled)
{
    // COP_ENGINE_PATCH: COP-ENG-064
    const float safe_capacity = _max(capacity, 0.f);
    const float safe_value = safe_capacity > EPS ? clampr(value, 0.f, safe_capacity) : 0.f;
    const float progress = safe_capacity > EPS ? safe_value / safe_capacity * 100.f : 0.f;
    UICarFuelBar->SetProgressPos(progress);

    const bool show_fuel = enabled && safe_capacity > EPS;
    UICarFuelText->Show(show_fuel);
    if (!show_fuel)
        return;

    const int current_liters = iFloor(safe_value + 0.5f);
    const int tank_liters = iFloor(safe_capacity + 0.5f);
    string64 fuel_text;
    xr_sprintf(fuel_text, "%d / %d %s", current_liters, tank_liters, UIFuelUnit.c_str());
    UICarFuelText->SetText(fuel_text);
}

//////////////////////////////////////////////////////////////////////////

void CUICarPanel::SetSpeed(float speed)
{
    //	clamp(speed,0.f,1.f);
    //	UISpeedometer.SetValue(speed);
}

//////////////////////////////////////////////////////////////////////////

void CUICarPanel::SetRPM(float rpm)
{
    //	clamp(rpm,0.f,1.f);
    //	UITachometer.SetValue(rpm);
}

void CUICarPanel::SetSpeedMode(const xr_string& Mode)
{
    UIEngineSpeedMode->SetText(Mode.c_str());
}

// COP_ENGINE_PATCH: COP-ENG-206 - the count is the actor's real remaining rounds.
void CUICarPanel::SetWeaponAmmo(bool visible, LPCSTR name, u32 count)
{
    UIWeaponAmmo->Show(visible);
    if (!visible) return;
    string256 text;
    xr_sprintf(text, "%s: %u", name, count);
    UIWeaponAmmo->SetText(text);
}
