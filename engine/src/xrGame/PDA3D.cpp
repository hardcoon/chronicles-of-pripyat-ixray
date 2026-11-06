#include "StdAfx.h"
#include "PDA.h"
#include "Actor.h"
#include "Inventory.h"
#include "Level.h"
#include "MainMenu.h"
#include "ui/UIGameCustom.h"
#include "ui/UIPdaWnd.h"
#include "../../xrUI/UICursor.h"
#include "../../xrUI/ui_base.h"
#include "../xrEngine/FontManager.h"
#include "../xrEngine/XR_IOConsole.h"
#include "../xrEngine/bone.h"

extern xrCriticalSection ui_lock;
ENGINE_API extern float psHUD_FOV_def;

// COP_ENGINE_PATCH: COP-ENG-212 - Advanced X-Ray PDA adapted to IX-Ray HUD/UI.
// Optional C exports preserve the installed xrEngine/xrAbstractions/xrUI ABI.
// COP_ENGINE_PATCH: COP-ENG-215 - optional by default, including fresh profiles.
Flags32 psPda3D = {0};
// COP_ENGINE_PATCH: COP-ENG-214 - magnification only in cursor/reading mode.
float psPdaReadZoom = 1.7f; // COP_ENGINE_PATCH: COP-ENG-217
namespace
{
    using BeginPda = BOOL (__cdecl*)(float);
    using EndPda = void (__cdecl*)();
    BeginPda begin_pda = nullptr;
    EndPda end_pda = nullptr;
    using SetPdaBrightness = void (__cdecl*)(float);
    SetPdaBrightness set_pda_brightness = nullptr;
    using SetPdaHudPose = void (__cdecl*)(IKinematics*, IKinematics*, IKinematics*, const Fmatrix*, const Fmatrix*);
    SetPdaHudPose set_pda_hud_pose = nullptr;
    HMODULE renderer_module = nullptr;
    bool ResolvePdaRenderer()
    {
        const HMODULE module = GetModuleHandleA("xrRender_R4.dll");
        if (module != renderer_module)
        {
            renderer_module = module;
            set_pda_hud_pose = module ? reinterpret_cast<SetPdaHudPose>(GetProcAddress(module, "xrSetPdaHudPose")) : nullptr;
            begin_pda = module ? reinterpret_cast<BeginPda>(GetProcAddress(module, "xrPdaBegin")) : nullptr;
            end_pda = module ? reinterpret_cast<EndPda>(GetProcAddress(module, "xrPdaEnd")) : nullptr;
            set_pda_brightness = module ? reinterpret_cast<SetPdaBrightness>(GetProcAddress(module, "xrSetPdaBrightness")) : nullptr;
        }
        return begin_pda && end_pda && set_pda_brightness;
    }
}

bool CPda::CanUse3D() const
{
    return m_has3D && psPda3D.test(1) && IsGameTypeSingle() && ResolvePdaRenderer();
}

bool CPda::Is3DActive() const
{
    return m_has3D && GetState() != eHidden && H_Parent() && H_Parent() == Level().CurrentViewEntity();
}

void CPda::Toggle3D()
{
    if (!m_pInventory || !CanUse3D()) return;
    if (Is3DActive() || m_pInventory->GetNextActiveSlot() == PDA_SLOT)
    {
        Close3D();
        return;
    }
    m_previousSlot = m_pInventory->GetActiveSlot();
    if (m_previousSlot == PDA_SLOT) m_previousSlot = NO_ACTIVE_SLOT;
    m_pInventory->Activate(PDA_SLOT);
    if (strstr(Core.Params, "-pda_trace"))
        Msg("[PDA3D] request previous=%u active=%u next=%u", m_previousSlot,
            m_pInventory->GetActiveSlot(), m_pInventory->GetNextActiveSlot());
}

void CPda::Capture3DCursor(bool capture)
{
    if (m_cursorCaptured == capture) return;
    m_cursorCaptured = capture;
    if (capture) Device.seqRender.Remove(&GetUICursor());
    else Device.seqRender.Add(&GetUICursor(), -1023);
}

void CPda::Close3D()
{
    m_readZoom.reset();
    Capture3DCursor(false);
    if (m_pInventory && m_pInventory->GetNextActiveSlot() == PDA_SLOT)
    {
        const u16 previous = m_previousSlot;
        m_previousSlot = NO_ACTIVE_SLOT;
        m_pInventory->Activate(previous != NO_ACTIVE_SLOT && m_pInventory->ItemFromSlot(previous) ? previous : NO_ACTIVE_SLOT);
    }
}

void CPda::Cleanup3D()
{
    m_readZoom.reset();
    Capture3DCursor(false);
    if (!m_has3D) return;
    if (CurrentGameUI() && Is3DActive() && CurrentGameUI()->PdaMenu().IsShown())
        CurrentGameUI()->PdaMenu().HideDialog();
    if (HudItemData())
    {
        if (m_joystickBone != BI_NONE) HudItemData()->m_model->LL_GetBoneInstance(m_joystickBone).reset_callback();
        g_player_hud->detach_item(this);
    }
    m_joystickBone = BI_NONE;
    StopCurrentAnimWithoutCallback();
    SetState(eHidden); SetNextState(eHidden); SetPending(FALSE);
    m_screenFadeFrom = m_zoomFactor = 0.f;
    m_previousSlot = NO_ACTIVE_SLOT;
}

void CPda::OnActiveItem()
{
    if (CanUse3D()) SwitchState(eShowing);
    else if (m_pInventory) m_pInventory->Activate(NO_ACTIVE_SLOT);
}

void CPda::OnHiddenItem()
{
    if (m_has3D && GetState() != eHidden) SwitchState(eHiding);
}

void CPda::OnStateSwitch(u32 state)
{
    if (!m_has3D) return;
    if (strstr(Core.Params, "-pda_trace"))
        Msg("[PDA3D] state %u -> %u active=%u next=%u", GetState(), state,
            m_pInventory ? m_pInventory->GetActiveSlot() : 0, m_pInventory ? m_pInventory->GetNextActiveSlot() : 0);
    // Remember the visible level if the player reverses a power transition.
    if (state == eShowing || state == eHiding)
        m_screenFadeFrom = ScreenBrightness();
    inherited::OnStateSwitch(state);
    if (!H_Parent() || !H_Parent()->cast_actor()) return;
    switch (state)
    {
    case eShowing:
        m_readZoom.reset();
        g_player_hud->attach_item(this);
        m_joystickRotation.set(0,0,0); m_joystickTarget.set(0,0,0); m_buttonPress = 0.f;
        m_screenSwitchAt = Device.fTimeGlobal + m_screenOnDelay;
        PlaySound("sndShow", Position());
        if (CurrentGameUI() && !CurrentGameUI()->PdaMenu().IsShown()) {
            auto& menu = CurrentGameUI()->PdaMenu();
            menu.ShowDialog(false);
            // Pre-render the interface during draw with the camera still under player control.
            menu.Enable(false);
            GetUICursor().Hide();
        }
        SetPending(TRUE);
        if (!PlayHUDMotion("anm_show", EHudMixType::eNoMix, state)) OnAnimationEnd(state);
        break;
    case eHiding:
        m_readZoom.reset();
        Capture3DCursor(false);
        if (CurrentGameUI() && CurrentGameUI()->PdaMenu().IsShown()) CurrentGameUI()->PdaMenu().HideDialog();
        m_screenSwitchAt = Device.fTimeGlobal + m_screenOffDelay;
        PlaySound("sndHide", Position());
        SetPending(TRUE);
        if (!PlayHUDMotion("anm_hide", EHudMixType::eMixAll, state)) OnAnimationEnd(state);
        break;
    case eIdle:
        SetPending(FALSE);
        PlayAnimIdle();
        if (HudItemData())
        {
            m_joystickBone = HudItemData()->m_model->LL_BoneID("joystick");
            if (m_joystickBone != BI_NONE)
                HudItemData()->m_model->LL_GetBoneInstance(m_joystickBone).set_callback(bctCustom, JoystickCallback, this);
        }
        break;
    case eHidden:
        Cleanup3D();
        break;
    }
}

void CPda::OnAnimationEnd(u32 state)
{
    if (state == eShowing) SwitchState(eIdle);
    else if (state == eHiding) SwitchState(eHidden);
    else inherited::OnAnimationEnd(state);
}

void CPda::OnMoveToRuck(const SInvItemPlace& previous)
{
    Cleanup3D();
    CInventoryItemObject::OnMoveToRuck(previous);
}

void CPda::UpdateCL()
{
    if (!m_has3D) { CInventoryItemObject::UpdateCL(); return; }
    inherited::UpdateCL();
    const bool canRead = Is3DActive() && GetState() == eIdle && CanUse3D() && CurrentGameUI() &&
        CurrentGameUI()->PdaMenu().IsShown() && CurrentGameUI()->HasPdaInput() &&
        Device.b_is_Active && !Console->bVisible;
    if (IsReading() && (!canRead || (H_Parent()->cast_actor()->GetMovementState(eWishful) & mcSprint)))
        SetReadMode(false);
    m_readZoom.update(Device.fTimeDelta, canRead, psPdaReadZoom);
    if (!Is3DActive()) return;
    if (!CanUse3D() || !H_Parent()->cast_actor()->g_Alive() ||
        (GetState() == eIdle && CurrentGameUI() && (!CurrentGameUI()->PdaMenu().IsShown() ||
            (!CurrentGameUI()->HasPdaInput() && !CurrentGameUI()->IsSearchingContainer())))) Close3D();
    // Disabled dialogs are skipped by CDialogHolder::OnFrame. Their texture still needs live UI.
    if (CurrentGameUI() && CurrentGameUI()->PdaMenu().IsShown() && !CurrentGameUI()->PdaMenu().IsEnabled()) {
        xrCriticalSectionGuard guard(ui_lock);
        CurrentGameUI()->PdaMenu().Update();
    }
    m_joystickRotation.lerp(m_joystickRotation, m_joystickTarget, _min(Device.fTimeDelta * 12.f, 1.f));
    m_joystickTarget.mul(_max(0.f, 1.f - Device.fTimeDelta * 10.f));
}

void CPda::UpdateHudAdditonal(Fmatrix& transform)
{
    auto* hud = HudItemData();
    if (!hud) return;
    const float target = GetState() == eIdle && IsReading() ? 1.f : 0.f;
    m_zoomFactor += (target - m_zoomFactor) * _min(1.f, Device.fTimeDelta * 8.f);
    // COP_ENGINE_PATCH: COP-ENG-217 - reach an exact rest pose instead of an asymptotic residual.
    if (_abs(target - m_zoomFactor) < .0001f) m_zoomFactor = target;
    ApplyHudAimOffset(transform, *hud);
}

void CPda::ApplyHudAimOffset(Fmatrix& transform, const attachable_hud_item& hud) const
{
    Fvector position = hud.m_measures.m_hands_positions.hands_offsets[0][1];
    Fvector rotation = hud.m_measures.m_hands_positions.hands_offsets[1][1];
    position.mul(m_zoomFactor); rotation.mul(m_zoomFactor);
    // Donor aim offsets use axis rotations. setHPB has the opposite sign.
    Fmatrix offset, part;
    offset.rotateX(rotation.x);
    part.rotateY(rotation.y); offset.mulA_43(part);
    part.rotateZ(rotation.z); offset.mulA_43(part);
    offset.translate_over(position);
    transform.mulB_43(offset);
}

// COP_ENGINE_PATCH: COP-ENG-221 - submit the settled pose directly in camera space.
// At reading distance, converting through distant world coordinates can move the image by pixels.
void CPda::SubmitReadPose(IKinematics* hands, IKinematics* watch, const Fmatrix& handOffset, const Fmatrix& itemBone)
{
    if (!set_pda_hud_pose || GetState() != eIdle || m_zoomFactor != 1.f) return;
    auto* actor = H_Parent() ? H_Parent()->cast_actor() : nullptr;
    if (!actor || (actor->GetMovementState(eReal) & (mcAnyAction | mcClimb | mcSprint))) return;
    auto* hud = HudItemData();
    if (!hud) return;

    Fmatrix handView = Fidentity;
    ApplyHudAimOffset(handView, *hud);
    handView.mulB_43(handOffset);
    Fmatrix itemView;
    itemView.mul_43(handView, itemBone);
    itemView.mulB_43(hud->m_attach_offset);
    set_pda_hud_pose(hands, hud->m_model, watch, &handView, &itemView);
}

void CPda::UpdateXForm() { CInventoryItem::UpdateXForm(); }

void CPda::SetReadMode(bool reading)
{
    if (reading && (!Is3DActive() || GetState() != eIdle || !CanUse3D() || !CurrentGameUI() ||
        CurrentGameUI()->TopInputReceiver() != &CurrentGameUI()->PdaMenu())) return;
    if (m_readZoom.reading == reading) return;
    m_readZoom.reading = reading;
    Capture3DCursor(reading); // The native 2D cursor belongs to any overlying loot dialog.
    if (CurrentGameUI()) {
        if (!reading) CurrentGameUI()->ClosePdaPopups();
        auto& menu = CurrentGameUI()->PdaMenu();
        menu.Enable(reading);
        if (CurrentGameUI()->TopInputReceiver() == &menu) {
            if (reading) GetUICursor().Show();
            else GetUICursor().Hide();
        }
    }
    if (reading && H_Parent() && H_Parent()->cast_actor())
        H_Parent()->cast_actor()->SetMovementState(eWishful, mcSprint, false);
    if (GetState() == eIdle) PlayAnimIdle();
}

bool CPda::Action(u16 cmd, u32 flags)
{
    if (!m_has3D || !Is3DActive()) return inherited::Action(cmd, flags);
    switch (cmd) {
    case kWPN_ZOOM:
    case kWPN_FIRE:
        if (flags & CMD_START) SetReadMode(true);
        return true;
    case kWPN_RELOAD:
        if (flags & CMD_START) SetReadMode(!IsReading());
        return true;
    default:
        return inherited::Action(cmd, flags);
    }
}

void CPda::PlayAnimIdle()
{
    if (!IsReading()) { inherited::PlayAnimIdle(); return; }
    if (!TryPlayAnimIdle()) PlayHUDMotion("anm_idle_aim", EHudMixType::eMixAll, GetState());
}
void CPda::PlayAnimIdleMoving() { PlayHUDMotion(IsReading() ? "anm_idle_aim_moving" : "anm_idle_moving", EHudMixType::eMixAll, GetState()); }
void CPda::PlayAnimIdleMovingSlow() { PlayHUDMotion(IsReading() ? "anm_idle_aim_moving_slow" : "anm_idle_moving_slow", EHudMixType::eMixAll, GetState()); }
void CPda::PlayAnimIdleMovingCrouch() { PlayHUDMotion(IsReading() ? "anm_idle_aim_moving_crouch" : "anm_idle_moving_crouch", EHudMixType::eMixAll, GetState()); }
void CPda::PlayAnimIdleMovingCrouchSlow() { PlayHUDMotion(IsReading() ? "anm_idle_aim_moving_crouch_slow" : "anm_idle_moving_crouch_slow", EHudMixType::eMixAll, GetState()); }

float CPda::GetHudFov()
{
    const float base = inherited::GetHudFov();
    // COP_ENGINE_PATCH: COP-ENG-217 - reading distance must not follow the near-wall ray.
    if (!m_has3D) return base;
    const float fixedBase = clampr(m_fHudFov ? m_fHudFov : psHUD_FOV_def, 5.f, 180.f) * m_fHudFovFactor;
    return m_readZoom.fov(base + (fixedBase - base) * m_zoomFactor);
}

void CPda::PdaMouse(float dx, float dy, bool pressed)
{
    m_joystickTarget.set(-dy * .015f, 0.f, -dx * .015f);
    clamp(m_joystickTarget.x, -.2f, .2f); clamp(m_joystickTarget.z, -.2f, .2f);
    const float target = pressed ? -.002f : 0.f;
    if (target != m_buttonPress) PlaySound(pressed ? "sndButtonPress" : "sndButtonRelease", Position());
    m_buttonPress = target;
}

void CPda::JoystickCallback(CBoneInstance* bone)
{
    auto* pda = static_cast<CPda*>(bone->callback_param());
    Fmatrix offset; offset.setHPB(pda->m_joystickRotation.y, pda->m_joystickRotation.x, pda->m_joystickRotation.z);
    offset.translate_over(0.f, pda->m_buttonPress, 0.f);
    bone->mTransform.mulB_43(offset);
}

// COP_ENGINE_PATCH: COP-ENG-219 - the power beep starts a 200 ms fade on the display only.
float CPda::ScreenBrightness() const
{
    const u32 state = GetState();
    if (state == eHidden) return 0.f;
    constexpr float fadeSeconds = .2f;
    const float progress = clampr((Device.fTimeGlobal - m_screenSwitchAt) / fadeSeconds, 0.f, 1.f);
    const float eased = progress * progress * (3.f - 2.f * progress);
    const float target = state == eHiding ? 0.f : 1.f;
    return m_screenFadeFrom + (target - m_screenFadeFrom) * eased;
}

void CPda::Render3DUI()
{
    // PDA tabs update on the game/UI worker; render their texture under the
    // same lock as CHUDManager::OnFrameMT.
    xrCriticalSectionGuard guard(ui_lock);
    if (!Is3DActive() || !CurrentGameUI() || !ResolvePdaRenderer()) return;
    // COP_ENGINE_PATCH: COP-ENG-217 - opt-in per-frame camera-relative pose evidence for regression QA.
    if (strstr(Core.Params, "-pda_pose_trace") && HudItemData()) {
        Fmatrix relative;
        relative.mul_43(Device.mView, HudItemData()->m_item_transform);
        Msg("[PDA_POSE] frame=%u reading=%d moving=%u pitch=%.9g zoom=%.9g aim=%.9g fov=%.9g motion=%s pos=(%.9g,%.9g,%.9g) i=(%.9g,%.9g,%.9g) j=(%.9g,%.9g,%.9g) k=(%.9g,%.9g,%.9g)",
            Device.dwFrame, IsReading(), Actor()->GetMovementState(eReal), Device.vCameraDirection.getP(),
            m_readZoom.factor, m_zoomFactor, GetHudFov(), m_current_motion.c_str(),
            VPUSH(relative.c), VPUSH(relative.i), VPUSH(relative.j), VPUSH(relative.k));
    }
    if (strstr(Core.Params, "-pda_trace"))
    {
        static u32 last = 0;
        if (Device.dwTimeGlobal - last > 2000)
        {
            last = Device.dwTimeGlobal;
            auto* hud = HudItemData();
            Fvector view = {};
            if (hud) Device.mView.transform_tiny(view, hud->m_item_transform.c);
            Msg("[PDA3D] render state=%u attached=%d HUDview=%d fov=%f view=(%f,%f,%f) UI=%d read_zoom=%f reading=%d input=%d cursor=%d aim=%f",
                GetState(), hud != nullptr, Actor()->HUDview(), GetHudFov(), view.x, view.y, view.z,
                CurrentGameUI()->PdaMenu().IsShown(), m_readZoom.factor, IsReading(),
                CurrentGameUI()->PdaMenu().IR_process(), GetUICursor().IsVisible(), m_zoomFactor);
        }
    }
    auto& menu = CurrentGameUI()->PdaMenu();
    const float brightness = ScreenBrightness();
    if (!menu.IsShown()) {
        // Holstering releases input immediately. Fade the last rendered page on the mesh.
        set_pda_brightness(brightness);
        return;
    }
    if (!begin_pda(brightness)) return;
    // COP_ENGINE_PATCH: COP-ENG-216 - match the display's small border crop.
    // Cursor and controls share the same texture, hence hit coordinates stay native.
    // COP_ENGINE_PATCH: COP-ENG-223 - page, modals and cursor share one texture and coordinate system.
    const bool deviceCursor = m_cursorCaptured && CurrentGameUI()->HasPdaInput() && !Console->bVisible;
    if (deviceCursor) {
        Fvector2 cursor = GetUICursor().GetCursorPosition();
        clamp(cursor.x, 14.f, 1010.f); clamp(cursor.y, 10.5f, 757.5f);
        GetUICursor().SetUICursorPosition(cursor);
    }
    menu.Draw3D();
    CurrentGameUI()->DrawPdaPopups();
    if (deviceCursor)
        GetUICursor().OnRender();
    UI().Font().Render();
    end_pda();
}
