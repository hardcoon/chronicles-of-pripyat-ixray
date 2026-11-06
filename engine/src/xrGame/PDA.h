#pragma once

#include "../xrEngine/Feel_Touch.h"
#include "hud_item_object.h"

#include "InfoPortionDefs.h"
#include "character_info_defs.h"

#include "PdaMsg.h"
#include "PdaReadZoom.h"

class CInventoryOwner;
class CPda;

using PDA_LIST = xr_vector<CPda*>;
using PDA_LIST_it = PDA_LIST::iterator;

class CPda :
	public CHudItemObject,
	public Feel::Touch
{
	typedef CHudItemObject inherited;
public:
											CPda					();
	virtual									~CPda					();

	virtual BOOL 							net_Spawn				(CSE_Abstract* DC);
	virtual void 							Load					(LPCSTR section);
	virtual void 							net_Destroy				();

	virtual void 							OnH_A_Chield			();
	virtual void 							OnH_B_Independent		(bool just_before_destroy);

	virtual void 							shedule_Update			(u32 dt);

	virtual void 							feel_touch_new			(CObject* O);
	virtual void 							feel_touch_delete		(CObject* O);
	virtual BOOL 							feel_touch_contact		(CObject* O);


	virtual u16								GetOriginalOwnerID		() {return m_idOriginalOwner;}
	virtual CInventoryOwner*				GetOriginalOwner		();
	virtual CObject*						GetOwnerObject			();


			void							TurnOn					() {m_bTurnedOff = false;}
			void							TurnOff					() {m_bTurnedOff = true;}
	
			bool 							IsActive				() {return IsOn();}
			bool 							IsOn					() {return !m_bTurnedOff;}
			bool 							IsOff					() {return m_bTurnedOff;}


			void							ActivePDAContacts		(xr_vector<CInventoryOwner*>& res);
			CInventoryOwner*				GetOwner				(CObject* owner);
			u32								ActiveContactsNum		()							{return (u32)m_active_contacts.size();}
			xr_vector<CObject*>				ActiveContacts			()							{return m_active_contacts;}
			void							PlayScriptFunction		();
			bool							CanPlayScriptFunction	() {if(!xr_strcmp(m_functor_str, "")) return false; return true;};


	virtual void							save					(NET_Packet &output_packet);
	virtual void							load					(IReader &input_packet);

	virtual CPda* cast_pda() { return this; }

//*	virtual LPCSTR							Name					();

// COP_ENGINE_PATCH: COP-ENG-212 - transient HUD state never enters PDA saves.
    bool CanUse3D() const;
    bool Is3DActive() const;
    void Toggle3D();
    void Close3D();
    void Cleanup3D();
    void Render3DUI();
    void SubmitReadPose(IKinematics* hands, IKinematics* watch, const Fmatrix& handOffset, const Fmatrix& itemBone);
    void PdaMouse(float dx, float dy, bool pressed);
    // COP_ENGINE_PATCH: COP-ENG-214 - Anomaly-style carrying and reading are separate modes.
    void SetReadMode(bool reading);
    bool IsReading() const { return m_readZoom.reading; }
    // COP_ENGINE_PATCH: COP-ENG-217 - blend only this item's procedural offsets out for reading.
    float ReadPoseFactor() const { return m_zoomFactor; }
    bool Action(u16 cmd, u32 flags) override;
    float GetHudFov() override;
    void PlayAnimIdle() override;
    void PlayAnimIdleMoving() override;
    void PlayAnimIdleMovingSlow() override;
    void PlayAnimIdleMovingCrouch() override;
    void PlayAnimIdleMovingCrouchSlow() override;
    CHudItem* cast_hud_item() override { return m_has3D ? this : nullptr; }
    void UpdateXForm() override;
    void UpdateCL() override;
    void OnActiveItem() override;
    void OnHiddenItem() override;
    void OnStateSwitch(u32 state) override;
    void OnAnimationEnd(u32 state) override;
    void OnMoveToRuck(const SInvItemPlace& previous) override;
    void UpdateHudAdditonal(Fmatrix& transform) override;
    bool AllowBore() override { return false; }
    u8 GetCurrentHudOffsetIdx() const override { return IsReading() ? 1 : 0; }
private:
    void ApplyHudAimOffset(Fmatrix& transform, const attachable_hud_item& hud) const;
    float ScreenBrightness() const;
    void Capture3DCursor(bool capture);
    static void JoystickCallback(CBoneInstance* bone);
    bool m_has3D = false, m_cursorCaptured = false;
    PdaReadZoom m_readZoom;
    u16 m_previousSlot = NO_ACTIVE_SLOT, m_joystickBone = BI_NONE;
    float m_screenFadeFrom = 0.f, m_zoomFactor = 0.f, m_screenSwitchAt = 0.f;
    float m_screenOnDelay = .68f, m_screenOffDelay = .67f, m_buttonPress = 0.f;
    Fvector m_joystickRotation = {}, m_joystickTarget = {};
protected:
	void									UpdateActiveContacts	();


	xr_vector<CObject*>						m_active_contacts;
	float									m_fRadius;

	u16										m_idOriginalOwner;
	shared_str								m_SpecificChracterOwner;
	xr_string								m_sFullName;

	bool									m_bTurnedOff;
	shared_str								m_functor_str;
};
