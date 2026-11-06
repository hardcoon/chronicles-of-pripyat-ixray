#pragma once

#include "../../xrUI/Widgets/UIStatic.h"
#include "../../xrUI/Widgets/UIProgressBar.h"

class CUICarPanel : public CUIWindow
{
private:
    typedef CUIWindow inherited;

    CUIStatic* UIStaticCarHealth;
    CUIProgressBar* UICarHealthBar;
    CUIProgressBar* UICarFuelBar;
    CUITextWnd* UICarFuelText;
    // COP_ENGINE_PATCH: COP-ENG-206 - uses the existing panel font/scaling.
    CUITextWnd* UIWeaponAmmo;
    xr_string UIFuelUnit;

    CUITextWnd* UIEngineSpeedMode;
    CUIStatic* UIEngineLamp;
    CUIStatic* UIEngineLampOff;

    CUIStatic* UILightLamp;
    CUIStatic* UILightLampOff;
	CUIStatic* UIConditionIcon;

public:
    // Установить
    ~CUICarPanel();

    void SetCarHealth(float value, float warning_threshold, float critical_threshold);
    void SetCarFuel(float value, float capacity, bool enabled);
    void SetWeaponAmmo(bool visible, LPCSTR name, u32 count);
    void SetSpeed(float speed);
    void SetRPM(float rmp);
    void SetSpeedMode(const xr_string& Mode);
    void Init();
    void SetEngineLamp(bool On);
    void SetLightLamp(bool On);
    virtual void Draw() override;

    virtual CUIWindow* ui_cast_window() { return this; }
};
