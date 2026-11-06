#include "StdAfx.h"

#ifdef DEBUG

#include "PHDebug.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "Actor.h"
#include "cameralook.h"
#include "CameraFirstEye.h"
#include "script_entity_action.h"
#include "../xrEngine/xr_level_controller.h"
#include "../Include/xrRender/Kinematics.h"
#include "Level.h"
#include "CarWeapon.h"

void CCar::OnMouseMove(int dx, int dy)
{
	if (!IsMyCar())
		return;

	CCameraBase* C = active_camera;
	float scale = (C->f_fov / g_fov) * psMouseSens * psMouseSensScale / 50.f;
	if (m_dcp_white_niva_profile)
		scale *= m_camera_angle_factor;
	if (dx)
	{
		float d = float(dx) * scale;
		C->Move((d < 0) ? kLEFT : kRIGHT, _abs(d));
	}

	if (dy)
	{
		float d = ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * 3.f / 4.f;
		C->Move((d > 0) ? kUP : kDOWN, _abs(d));
	}
}

bool CCar::bfAssignMovement(CScriptEntityAction *tpEntityAction)
{
	if (tpEntityAction->m_tMovementAction.m_bCompleted)
		return(false);

	u32		l_tInput = tpEntityAction->m_tMovementAction.m_tInputKeys;

	vfProcessInputKey(kFWD		,	!!(l_tInput & CScriptMovementAction::eInputKeyForward		));
	vfProcessInputKey(kBACK		,	!!(l_tInput & CScriptMovementAction::eInputKeyBack		));
	vfProcessInputKey(kL_STRAFE	,	!!(l_tInput & CScriptMovementAction::eInputKeyLeft		));
	vfProcessInputKey(kR_STRAFE	,	!!(l_tInput & CScriptMovementAction::eInputKeyRight		));
	vfProcessInputKey(kACCEL	,	!!(l_tInput & CScriptMovementAction::eInputKeyShiftUp		));
	vfProcessInputKey(kCROUCH	,	!!(l_tInput & CScriptMovementAction::eInputKeyShiftDown	));
	vfProcessInputKey(kJUMP		,	!!(l_tInput & CScriptMovementAction::eInputKeyBreaks		));
	if (!!(l_tInput & CScriptMovementAction::eInputKeyEngineOn))	StartEngine();
	if (!!(l_tInput & CScriptMovementAction::eInputKeyEngineOff)) StopEngine();

	//if (_abs(tpEntityAction->m_tMovementAction.m_fSpeed) > EPS_L)
		//m_current_rpm = _abs(tpEntityAction->m_tMovementAction.m_fSpeed*m_current_gear_ratio);

	return	(true);
}

bool CCar::bfAssignObject(CScriptEntityAction *tpEntityAction)
{
	CScriptObjectAction	&l_tObjectAction = tpEntityAction->m_tObjectAction;
	if (l_tObjectAction.m_bCompleted || !xr_strlen(l_tObjectAction.m_caBoneName))
		return((l_tObjectAction.m_bCompleted = true) == false);

	s16	l_sBoneID = smart_cast<IKinematics*>(Visual())->LL_BoneID(l_tObjectAction.m_caBoneName);
	if (is_Door(l_sBoneID)) {
		switch(l_tObjectAction.m_tGoalType) {
			case MonsterSpace::eObjectActionActivate : {
				if (!DoorOpen(l_sBoneID))
					return((l_tObjectAction.m_bCompleted = true) == false);
				break;
			}
			case MonsterSpace::eObjectActionDeactivate : {
				if (!DoorClose(l_sBoneID))
					return((l_tObjectAction.m_bCompleted = true) == false);
				break;
			}
			case MonsterSpace::eObjectActionUse : {
				if (!DoorSwitch(l_sBoneID))
					return((l_tObjectAction.m_bCompleted = true) == false);
				break;
			}
			default : 
				return	((l_tObjectAction.m_bCompleted = true) == false);
		}
		return		(false);
	}
	SCarLight* light=nullptr;
	if (m_lights.findLight(l_sBoneID,light)) {
		switch(l_tObjectAction.m_tGoalType) {
			case MonsterSpace::eObjectActionActivate : {
				light->TurnOn();
				return		((l_tObjectAction.m_bCompleted = true) == false);
			}
			case MonsterSpace::eObjectActionDeactivate : {
				light->TurnOff();
				return		((l_tObjectAction.m_bCompleted = true) == false);
			}
			case MonsterSpace::eObjectActionUse : {
				light->Switch();
				return		((l_tObjectAction.m_bCompleted = true) == false);
			}
			default : 
				return	((l_tObjectAction.m_bCompleted = true) == false);
		}
	
	}
	
	return			(false);
}

void CCar::vfProcessInputKey	(int iCommand, bool bPressed)
{
	if (bPressed)
		OnKeyboardPress			(iCommand);
	else
		OnKeyboardRelease		(iCommand);
}

void CCar::OnKeyboardPress(int cmd)
{
	if (!IsMyCar() && !g_dedicated_server)
		return;
	// COP_ENGINE_PATCH: COP-ENG-203 - reuse the user's native slot bindings.
	if (cmd == kWPN_1 || cmd == kWPN_2)
	{
		SwitchVehicleRole(cmd == kWPN_2);
		return;
	}
	// COP_ENGINE_PATCH: COP-ENG-205 - native view bindings work inside the
	// operator context without changing role, enabling drive or losing the gun.
	if (IsWeaponMode() && (cmd == kCAM_1 || cmd == kCAM_2 || cmd == kCAM_3))
	{
		SwitchWeaponView(cmd == kCAM_3 || (cmd == kCAM_2 && !IsWeaponExternalView()));
		return;
	}
	if (IsWeaponMode() && (cmd == kFWD || cmd == kBACK || cmd == kL_STRAFE || cmd == kR_STRAFE ||
		cmd == kACCEL || cmd == kCROUCH || cmd == kJUMP))
		return;

	switch (cmd)	
	{
	case kCAM_1:	OnCameraChange(ectFirst);	break;
	case kCAM_2:
		if (active_camera->tag != ectChase)
			OnCameraChange(ectChase);
		else
			OnCameraChange(ectFirst);
		break;
	case kCAM_3:	OnCameraChange(ectFree);	break;
	case kACCEL:	TransmissionUp();			break;
	case kCROUCH:	TransmissionDown();			break;
	case kFWD:		PressForward();				break;
	case kBACK:		PressBack();				break;
	case kR_STRAFE:	PressRight();				if (OwnerActor()) OwnerActor()->steer_Vehicle(1);	break;
	case kL_STRAFE:	PressLeft();				if (OwnerActor()) OwnerActor()->steer_Vehicle(-1);break;
	case kJUMP:		PressBreaks();				break;
	case kDETECTOR: SwitchEngine();				break;
	case kTORCH:	m_lights.SwitchHeadLights();break;
	case kUSE:									break;
	// COP_ENGINE_PATCH: COP-ENG-201 - existing fire binding, only in an already
	// active mounted-weapon context. Mode activation/switching is a separate task.
	case kWPN_FIRE:
		if (m_car_weapon && m_car_weapon->UsesProfileInput() && m_car_weapon->IsActive())
			m_car_weapon->Action(CCarWeapon::eWpnFire, 1);
		break;
	};

	if (OnClient())
	{
		NET_Packet P;
		CGameObject::u_EventGen(P, GE_GAME_EVENT, Owner()->ID());
		P.w_u16(GAME_EVENT_MP_CAR_INPUT);
		P.w_u16(ID());
		P.w_u8(cmd);
		P.w_u8(true);
		CGameObject::u_EventSend(P);
	}
}

void CCar::OnKeyboardRelease(int cmd)
{
	if (!IsMyCar() && !g_dedicated_server)
		return;
	// COP_ENGINE_PATCH: COP-ENG-203 - a key held before role switching must
	// not release the operator's parking brake after its press was discarded.
	if (IsWeaponMode() && (cmd == kFWD || cmd == kBACK || cmd == kL_STRAFE || cmd == kR_STRAFE || cmd == kJUMP))
		return;

	switch (cmd)	
	{
	case kACCEL:break;
	case kFWD:		ReleaseForward();			break;
	case kBACK:		ReleaseBack();				break;
	case kL_STRAFE:	ReleaseLeft();				if (OwnerActor()) OwnerActor()->steer_Vehicle(0);	break;
	case kR_STRAFE:	ReleaseRight();				if (OwnerActor()) OwnerActor()->steer_Vehicle(0);	break;
	case kJUMP:		ReleaseBreaks();			break;
	case kWPN_FIRE:
		if (m_car_weapon && m_car_weapon->UsesProfileInput())
			m_car_weapon->Action(CCarWeapon::eWpnFire, 0);
		break;
	};

	if (OnClient())
	{
		NET_Packet P;
		CGameObject::u_EventGen(P, GE_GAME_EVENT, Owner()->ID());
		P.w_u16(GAME_EVENT_MP_CAR_INPUT);
		P.w_u16(ID());
		P.w_u8(cmd);
		P.w_u8(false);
		CGameObject::u_EventSend(P);
	}
}

void CCar::OnKeyboardHold(int cmd)
{
	if (!IsMyCar())
		return;

	switch(cmd)
	{
	case kCAM_ZOOM_IN: 
	case kCAM_ZOOM_OUT: 
	case kUP:
	case kDOWN:
	case kLEFT:
	case kRIGHT:	active_camera->Move(cmd);	break;
	}
}
void CCar::Action(u16 id, u32 flags)
{
	if (id == CCarWeapon::eWpnActivate && OwnerActor() && m_car_weapon && m_car_weapon->UsesProfileInput())
	{
		SwitchVehicleRole(flags == 1);
		return;
	}
	if(m_car_weapon)m_car_weapon->Action(id,flags);
}

bool CCar::IsWeaponMode() const
{
	return (active_camera == m_weapon_camera || active_camera == m_weapon_external_camera) &&
		m_car_weapon && m_car_weapon->UsesProfileInput() && m_car_weapon->IsActive();
}

bool CCar::IsWeaponExternalView() const
{
	return IsWeaponMode() && active_camera == m_weapon_external_camera;
}

void CCar::SwitchWeaponView(bool external)
{
	if (!IsWeaponMode() || (external && !m_car_weapon->SupportsExternalView())) return;
	CCameraBase* next = external ? m_weapon_external_camera : m_weapon_camera;
	if (next == active_camera) return;
	m_car_weapon->ConfigureOperatorCamera(*next, external);
	active_camera = next;
	ResetVehicleCameraDynamics();
}

bool CCar::SwitchVehicleRole(bool gunner)
{
	// COP_ENGINE_PATCH: COP-ENG-203 - role switching never detaches the actor.
	if (!OwnerActor() || !m_car_weapon || !m_car_weapon->UsesProfileInput()) return false;
	if (gunner && IsWeaponMode()) return true;
	if (!gunner && active_camera != m_weapon_camera && active_camera != m_weapon_external_camera) return true;
	ReleaseForward(); ReleaseBack(); ReleaseLeft(); ReleaseRight(); ReleaseBreaks();
	NeutralDrive(); Unclutch(); ResetKeys();
	OwnerActor()->steer_Vehicle(0);
	if (gunner)
	{
		m_driver_camera_type = active_camera->tag;
		m_car_weapon->ConfigureOperatorCamera(*m_weapon_camera);
		m_car_weapon->Action(CCarWeapon::eWpnActivate, 1);
		active_camera = m_weapon_camera;
		Owner()->setVisible(FALSE);
		HandBreak();
	}
	else
	{
		m_car_weapon->Action(CCarWeapon::eWpnActivate, 0);
		active_camera = camera[m_driver_camera_type];
		OnCameraChange(m_driver_camera_type);
		Owner()->setVisible(m_driver_camera_type != ectFirst);
		ReleaseHandBreak();
	}
	ResetVehicleCameraDynamics();
	return true;
}
void CCar::SetParam(int id, Fvector2 val)
{
	if(m_car_weapon)m_car_weapon->SetParam(id,val);
}
void CCar::SetParam			(int id, Fvector val)
{
	if(m_car_weapon)m_car_weapon->SetParam(id,val);
}
bool CCar::WpnCanHit()
{
	if(m_car_weapon) return m_car_weapon->AllowFire();
	return false;
}

float CCar::FireDirDiff()
{
	if(m_car_weapon) return m_car_weapon->FireDirDiff();
	return 0.0f;
}
#include "script_game_object.h"
#include "car_memory.h"
#include "visual_memory_manager.h"

bool CCar::isObjectVisible			(CScriptGameObject* O_)
{
	if(m_memory)
	{
		return m_memory->visual().visible_now(&O_->object());
	}else
	{

		if(!O_)
		{
			Msg("Attempt to call CCar::isObjectVisible method wihth passed nullptr parameter");
			return false;
		}
		CObject* O = &O_->object();
		Fvector dir_to_object;
		Fvector to_point;
		O->Center(to_point);
		
		Fvector				from_point;
		Center				(from_point);
	
		if(HasWeapon())
		{
			from_point.y		= XFORM().c.y + m_car_weapon->_height();
		}

		dir_to_object.sub(to_point,from_point).normalize_safe();
		float ray_length = from_point.distance_to(to_point);


		BOOL res = Level().ObjectSpace.RayTest(from_point, dir_to_object, ray_length, collide::rqtStatic, nullptr, nullptr);
		return (0==res);
	}
}

bool CCar::HasWeapon()
{
	return (m_car_weapon != nullptr);
}

Fvector CCar::CurrentVel()
{
	Fvector lin_vel;
	m_pPhysicsShell->get_LinearVel(lin_vel);

	return lin_vel;
}

// COP_ENGINE_PATCH: COP-ENG-206 - read-only native count for scripts and QA.
u32 CCar::WeaponInventoryAmmo() const
{
	return m_car_weapon ? m_car_weapon->InventoryAmmoCount() : 0;
}
