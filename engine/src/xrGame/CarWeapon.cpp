#include "StdAfx.h"
#include "CarWeapon.h"
#include "../xrPhysics/PhysicsShell.h"
#include "PhysicsShellHolder.h"
#include "../Include/xrRender/Kinematics.h"
#include "object_broker.h"
#include "../xrSound/ai_sounds.h"
#include "WeaponAmmo.h"
#include "../xrEngine/xr_level_controller.h"
#include "../xrEngine/bone.h"
#include "game_object_space.h"
#include "../xrEngine/CameraBase.h"
#include "cameralook.h"
#include "Car.h"
#include "Actor.h"
#include "Inventory.h"
#include "Level.h"

void CCarWeapon::BoneCallbackX		(CBoneInstance *B)
{
	CCarWeapon	*P = static_cast<CCarWeapon*>(B->callback_param());
	Fmatrix rX;		rX.rotateX		(P->m_cur_x_rot);
	B->mTransform.mulB_43			(rX);
}

void CCarWeapon::BoneCallbackY		(CBoneInstance *B)
{
	CCarWeapon	*P = static_cast<CCarWeapon*>(B->callback_param());
	Fmatrix rY;		rY.rotateY		(P->m_cur_y_rot);
	B->mTransform.mulB_43			(rY);
}

CCarWeapon::CCarWeapon(CPhysicsShellHolder* obj)
{
	m_bActive	= false;
	m_bAutoFire	= false;
	m_allow_fire = false;
	m_physical_aim = false;
	m_object	= obj;
	m_Ammo		= new CCartridge();

	IKinematics* K			= smart_cast<IKinematics*>(m_object->Visual());
	CInifile* pUserData		= K->LL_UserData(); 
	// COP_ENGINE_PATCH: COP-ENG-201 - opt-in keeps existing non-DCP weapons unchanged.
	m_physical_aim = xr_strcmp(READ_IF_EXISTS(pUserData, r_string,
		"mounted_weapon_definition", "weapon_feature_profile", ""), "dcp_vehicle_weapon_v1") == 0;
	m_profile_input = m_physical_aim || xr_strcmp(READ_IF_EXISTS(pUserData, r_string,
		"mounted_weapon_definition", "weapon_feature_profile", ""), "dcp_vehicle_weapon_visual_v1") == 0;
	// COP_ENGINE_PATCH: COP-ENG-203 - effective donor offset behind the actual barrel.
	m_operator_camera_offset = READ_IF_EXISTS(pUserData, r_fvector3, "mounted_weapon_definition", "cam_pos_rel", Fvector().set(0.f, .1f, -1.2f));
	// COP_ENGINE_PATCH: COP-ENG-205 - data opt-in, independent of the barrel pitch.
	m_operator_external_enabled = m_profile_input && READ_IF_EXISTS(pUserData, r_bool,
		"mounted_weapon_definition", "cam_external_enabled", false);
	m_operator_external_offset = READ_IF_EXISTS(pUserData, r_fvector3, "mounted_weapon_definition",
		"cam_external_pos_rel", Fvector().set(0.f, .7f, 0.f));
	m_operator_external_zoom = READ_IF_EXISTS(pUserData, r_fvector2, "mounted_weapon_definition",
		"cam_external_zoom", Fvector2().set(4.f, 10.f));
	m_pitch_torque = _max(READ_IF_EXISTS(pUserData, r_float, "mounted_weapon_definition", "pitch_torque", 2500.f), 1.f);
	m_yaw_torque = _max(READ_IF_EXISTS(pUserData, r_float, "mounted_weapon_definition", "yaw_torque", 5000.f), 1.f);

	m_rotate_x_bone			= K->LL_BoneID	(pUserData->r_string("mounted_weapon_definition","rotate_x_bone"));
	m_rotate_y_bone			= K->LL_BoneID	(pUserData->r_string("mounted_weapon_definition","rotate_y_bone"));
	m_fire_bone				= K->LL_BoneID	(pUserData->r_string("mounted_weapon_definition","fire_bone"));
	m_camera_bone = K->LL_BoneID(READ_IF_EXISTS(pUserData, r_string, "mounted_weapon_definition",
		"cam_bone", pUserData->r_string("mounted_weapon_definition", "fire_bone")));
	if (m_camera_bone == BI_NONE) m_camera_bone = m_fire_bone;
	m_min_gun_speed			= pUserData->r_float("mounted_weapon_definition","min_gun_speed");
	m_max_gun_speed			= pUserData->r_float("mounted_weapon_definition","max_gun_speed");
	CBoneData& bdX			= K->LL_GetData(m_rotate_x_bone); //VERIFY(bdX.IK_data.type==jtJoint);
	m_lim_x_rot.set			(bdX.IK_data.limits[0].limit.x,bdX.IK_data.limits[0].limit.y);
	CBoneData& bdY			= K->LL_GetData(m_rotate_y_bone); //VERIFY(bdY.IK_data.type==jtJoint);
	m_lim_y_rot.set			(bdY.IK_data.limits[1].limit.x,bdY.IK_data.limits[1].limit.y);
	

	xr_vector<Fmatrix>					matrices;
	K->LL_GetBindTransform				(matrices);
	m_i_bind_x_xform.invert				(matrices[m_rotate_x_bone]);
	m_i_bind_y_xform.invert				(matrices[m_rotate_y_bone]);
	m_bind_x_rot						= matrices[m_rotate_x_bone].k.getP();
	m_bind_y_rot						= matrices[m_rotate_y_bone].k.getH();
	m_bind_x.set						(matrices[m_rotate_x_bone].c);
	m_bind_y.set						(matrices[m_rotate_y_bone].c);

	m_cur_x_rot							= m_bind_x_rot;
	m_cur_y_rot							= m_bind_y_rot;
	m_destEnemyDir.setHP				(m_bind_y_rot,m_bind_x_rot);
	m_object->XFORM().transform_dir		(m_destEnemyDir);


	inheritedShooting::Light_Create		();
	Load								(pUserData->r_string("mounted_weapon_definition","wpn_section"));
	if (!m_physical_aim)
		SetBoneCallbacks();
	m_object->processing_activate		();

	m_weapon_h							= matrices[m_rotate_y_bone].c.y;
	m_fire_norm.set						(0,1,0);
	m_fire_dir.set						(0,0,1);
	m_fire_pos.set						(0,0,0);
	if (m_profile_input)
	{
		m_cur_x_rot = m_cur_y_rot = 0.f; // callbacks add deltas to the existing bind transform.
		K->CalculateBones_Invalidate();
		K->CalculateBones(TRUE);
		UpdateFireTransform();
		m_destEnemyDir.set(m_fire_dir);
		if (m_physical_aim) HoldPhysicalAim();
	}
}

CCarWeapon::~CCarWeapon()
{
	delete_data(m_Ammo);
//.	m_object->processing_deactivate		();
}

void CCarWeapon::Load(LPCSTR section)
{
	inheritedShooting::Load(section);
    SoundsLayered.LoadSound(section, "snd_shoot", "m_sndShot", false);
	m_Ammo->Load(pSettings->r_string(section, "ammo_class"), 0);
	// COP_ENGINE_PATCH: COP-ENG-206 - legacy mounts remain unchanged unless opted in.
	m_inventory_ammo = m_profile_input && xr_strcmp(READ_IF_EXISTS(pSettings, r_string,
		section, "ammo_feed_profile", ""), "actor_inventory_v1") == 0;
	if (m_inventory_ammo)
	{
		m_inventory_ammo_section = pSettings->r_string(section, "ammo_inventory_class");
	}

}

void CCarWeapon::UpdateCL()
{
	if(!m_bActive)				return;
	if (m_physical_aim)
	{
		// COP_ENGINE_PATCH: COP-ENG-201 - physical collision, visible barrel,
		// projectile, sound and flash use the same current skeleton transform.
		IKinematics* K = smart_cast<IKinematics*>(m_object->Visual());
		K->CalculateBones_Invalidate();
		K->CalculateBones(TRUE);
		UpdateFireTransform();
		UpdatePhysicalAim();
		UpdateFire();
		return;
	}
	if (m_profile_input)
	{
		// A donor mount without a physical barrel keeps its original visual joint.
		// Transform each axis independently; reusing the pitch-space vector for yaw
		// corrupts aiming on rotated vehicle and bind transforms.
		Fmatrix inverse;
		inverse.invert(m_object->XFORM());
		Fvector local, pitch, yaw;
		inverse.transform_dir(local, m_destEnemyDir);
		m_i_bind_x_xform.transform_dir(pitch, local);
		m_i_bind_y_xform.transform_dir(yaw, local);
		m_tgt_x_rot = clampr(-pitch.getP(), -m_lim_x_rot.y, -m_lim_x_rot.x);
		m_tgt_y_rot = clampr(angle_normalize_signed(-yaw.getH()), -m_lim_y_rot.y, -m_lim_y_rot.x);
		m_cur_x_rot = angle_inertion_var(m_cur_x_rot, m_tgt_x_rot, m_min_gun_speed, m_max_gun_speed, PI, Device.fTimeDelta);
		m_cur_y_rot = angle_inertion_var(m_cur_y_rot, m_tgt_y_rot, m_min_gun_speed, m_max_gun_speed, PI, Device.fTimeDelta);
		IKinematics* K = smart_cast<IKinematics*>(m_object->Visual());
		K->CalculateBones_Invalidate();
		K->CalculateBones(TRUE);
		UpdateFireTransform();
		m_allow_fire = m_fire_dir.dotproduct(m_destEnemyDir) >= cosf(deg2rad(5.f));
		UpdateFire();
		return;
	}
	UpdateBarrelDir				();
	IKinematics* K				= smart_cast<IKinematics*>(m_object->Visual());
	K->CalculateBones_Invalidate();
	K->CalculateBones			(TRUE);
	UpdateFire					();
}

void CCarWeapon::UpdateFire()
{
	fShotTimeCounter -= Device.fTimeDelta;

	inheritedShooting::UpdateFlameParticles();
	inheritedShooting::UpdateLight();
	
	if(m_bAutoFire){
		if(m_allow_fire){
			FireStart();
		}else
			FireEnd();
	};

	if(!IsWorking())
	{
		clamp(fShotTimeCounter, 0.0f, flt_max);
		return;
	}

	if(fShotTimeCounter<=0)
	{
		OnShot();
		fShotTimeCounter += fOneShotTime;
	}
}

void CCarWeapon::Render_internal()
{
	RenderLight		();	
}

void CCarWeapon::SetBoneCallbacks()
{
//	m_object->PPhysicsShell()->EnabledCallbacks(FALSE);
	
	CBoneInstance& biX		= smart_cast<IKinematics*>(m_object->Visual())->LL_GetBoneInstance(m_rotate_x_bone);	
	biX.set_callback		(bctCustom,BoneCallbackX,this);
	CBoneInstance& biY		= smart_cast<IKinematics*>(m_object->Visual())->LL_GetBoneInstance(m_rotate_y_bone);	
	biY.set_callback		(bctCustom,BoneCallbackY,this);
}

void CCarWeapon::ResetBoneCallbacks()
{
	CBoneInstance& biX		= smart_cast<IKinematics*>(m_object->Visual())->LL_GetBoneInstance(m_rotate_x_bone);	
	biX.reset_callback		();
	CBoneInstance& biY		= smart_cast<IKinematics*>(m_object->Visual())->LL_GetBoneInstance(m_rotate_y_bone);	
	biY.reset_callback		();

//	m_object->PPhysicsShell()->EnabledCallbacks(TRUE);
}

void CCarWeapon::UpdateBarrelDir()
{
	IKinematics* K		= smart_cast<IKinematics*>(m_object->Visual());
	m_fire_bone_xform	= K->LL_GetTransform(m_fire_bone);

	m_fire_bone_xform.mulA_43(m_object->XFORM());
	m_fire_pos.set(0,0,0); 
	m_fire_bone_xform.transform_tiny(m_fire_pos);
	m_fire_dir.set(0,0,1);
	m_fire_bone_xform.transform_dir(m_fire_dir);
	m_fire_norm.set(0,1,0);
	m_fire_bone_xform.transform_dir(m_fire_norm);


	m_allow_fire		= true;
	Fmatrix XFi;
	XFi.invert			(m_object->XFORM());
	Fvector dep;
	XFi.transform_dir	(dep,m_destEnemyDir);
	{// x angle
		m_i_bind_x_xform.transform_dir(dep); dep.normalize();
		m_tgt_x_rot		= angle_normalize_signed(m_bind_x_rot-dep.getP());
		clamp			(m_tgt_x_rot,-m_lim_x_rot.y,-m_lim_x_rot.x);
	}
	{// y angle
		m_i_bind_y_xform.transform_dir(dep); dep.normalize();
		m_tgt_y_rot		= angle_normalize_signed(m_bind_y_rot-dep.getH());
		clamp			(m_tgt_y_rot,-m_lim_y_rot.y,-m_lim_y_rot.x);
	}

	m_cur_x_rot		= angle_inertion_var(m_cur_x_rot,m_tgt_x_rot,m_min_gun_speed,m_max_gun_speed,PI,Device.fTimeDelta);
	m_cur_y_rot		= angle_inertion_var(m_cur_y_rot,m_tgt_y_rot,m_min_gun_speed,m_max_gun_speed,PI,Device.fTimeDelta);
	static float dir_eps = deg2rad(5.0f);
	if( !fsimilar(m_cur_x_rot,m_tgt_x_rot,dir_eps)|| !fsimilar(m_cur_y_rot,m_tgt_y_rot,dir_eps))
		m_allow_fire=FALSE;

#if (0)
	if(Device.dwFrame%200==0){
		Msg("m_cur_x_rot=[%f]",m_cur_x_rot);
		Msg("m_cur_y_rot=[%f]",m_cur_y_rot);
	}
#endif
}
bool CCarWeapon::AllowFire()
{
	return m_allow_fire;
}

float CCarWeapon::FireDirDiff()
{
	if (m_profile_input)
		return rad2deg(acosf(clampr(m_fire_dir.dotproduct(m_destEnemyDir), -1.f, 1.f)));
	Fvector d1,d2;
	d1.set(m_cur_x_rot,m_cur_y_rot,0).normalize_safe();
	d2.set(m_tgt_x_rot,m_tgt_y_rot,0).normalize_safe();
	return rad2deg( acos(d1.dotproduct(d2)) );
}

const Fvector&	CCarWeapon::get_CurrentFirePoint()
{
	return m_fire_pos;
}

const Fmatrix&	CCarWeapon::get_ParticlesXFORM	()						
{
	return m_fire_bone_xform;
}

void CCarWeapon::FireStart()
{
	inheritedShooting::FireStart();
}

void CCarWeapon::FireEnd()	
{
	inheritedShooting::FireEnd();
	StopFlameParticles	();
}

#include "holder_custom.h"
void CCarWeapon::OnShot()
{
	CHolderCustom* holder = smart_cast<CHolderCustom*>(m_object);

	// COP_ENGINE_PATCH: COP-ENG-206 - consume before emitting a bullet/effects.
	// Native ammo items own the remaining rounds and invalidate inventory weight.
	// Existing vehicle ballistics stay configured by its original weapon section.
	if (m_inventory_ammo && !TakeCartridge())
	{
		FireEnd();
		return;
	}
	CActor* gunner = m_inventory_ammo ? AmmoUser() : nullptr;
	const u16 parent = gunner ? gunner->ID() : (holder->Engaged() ? 0 : m_object->ID());
	FireBullet(m_fire_pos, m_fire_dir, fireDispersionBase, *m_Ammo, parent,
		m_object->ID(), SendHitAllowed(m_object));

	StartShotParticles();

	if (m_bLightShotEnabled)
		Light_Start();

	StartFlameParticles();
	StartSmokeParticles(m_fire_pos, zero_vel);

    SoundsLayered.PlaySound("m_sndShot", m_fire_pos, m_object, false);
}

void CCarWeapon::Action				(u16 id, u32 flags)
{
	switch (id){
		case eWpnFire:{
			if(flags==1)			FireStart	();
			else					FireEnd		();
		}break;
		case eWpnActivate:{
			if(flags==1)			m_bActive = true;
			else					{m_bActive = false; FireEnd(); if (m_physical_aim) HoldPhysicalAim();}
		}break;

		case eWpnAutoFire:{
			if(flags==1)			m_bAutoFire = true;
			else					m_bAutoFire = false;
		}break;
		case eWpnToDefaultDir:{
			SetParam(eWpnDesiredDir,Fvector2().set(m_bind_y_rot,m_bind_x_rot));
		}break;
			
	}
}

void CCarWeapon::SetParam			(int id, Fvector2 val)
{
	switch (id){
		case eWpnDesiredDir:
			m_destEnemyDir.setHP				(val.x,val.y);
		break;
	}
}

void CCarWeapon::SetParam			(int id, Fvector val)
{
	switch (id){
		case eWpnDesiredDir:
			if (m_profile_input && val.square_magnitude() > EPS_S)
				m_destEnemyDir.set(val).normalize();
			break;
		case eWpnDesiredPos:
			m_destEnemyDir.sub(val,m_fire_pos).normalize_safe();
		break;
	}
}

// COP_ENGINE_PATCH: COP-ENG-201 - no model-specific bone names or fake visual joints.
void CCarWeapon::UpdateFireTransform()
{
	IKinematics* K = smart_cast<IKinematics*>(m_object->Visual());
	m_fire_bone_xform.mul_43(m_object->XFORM(), K->LL_GetTransform(m_fire_bone));
	m_fire_pos.set(m_fire_bone_xform.c);
	m_fire_dir.set(m_fire_bone_xform.k).normalize_safe();
	m_fire_norm.set(m_fire_bone_xform.j).normalize_safe();
}

void CCarWeapon::HoldPhysicalAim()
{
	if (!m_object->PPhysicsShell())
		return;
	if (CPhysicsJoint* joint = m_object->PPhysicsShell()->get_Joint(m_rotate_x_bone))
		joint->SetForceAndVelocity(m_pitch_torque, 0.f, 0);
	if (CPhysicsJoint* joint = m_object->PPhysicsShell()->get_Joint(m_rotate_y_bone))
		joint->SetForceAndVelocity(m_yaw_torque, 0.f, 0);
}

void CCarWeapon::UpdatePhysicalAim()
{
	if (!m_object->PPhysicsShell())
		return;
	const u16 bones[] = { m_rotate_y_bone, m_rotate_x_bone };
	const float torques[] = { m_yaw_torque, m_pitch_torque };
	for (u32 index = 0; index < 2; ++index)
	{
		CPhysicsJoint* joint = m_object->PPhysicsShell()->get_Joint(bones[index]);
		if (!joint)
			continue;
		Fvector axis;
		joint->GetAxisDirDynamic(0, axis);
		axis.normalize_safe();
		Fvector current, desired;
		current.mad(m_fire_dir, axis, -m_fire_dir.dotproduct(axis));
		desired.mad(m_destEnemyDir, axis, -m_destEnemyDir.dotproduct(axis));
		if (current.square_magnitude() < EPS_S || desired.square_magnitude() < EPS_S)
		{
			joint->SetForceAndVelocity(torques[index], 0.f, 0);
			continue;
		}
		current.normalize();
		desired.normalize();
		Fvector cross;
		cross.crossproduct(current, desired);
		const float error = atan2f(axis.dotproduct(cross), clampr(current.dotproduct(desired), -1.f, 1.f));
		const float angle = joint->GetAxisAngle(0);
		float lo, hi;
		joint->GetLimits(lo, hi, 0);
		// COP_ENGINE_PATCH: COP-ENG-204 - a full-turn hinge reports a wrapped
		// angle. Subtracting two wrapped angles can command an almost 360-degree
		// reversal at the seam; the projected signed error already is the short arc.
		const float remaining = hi - lo >= 2.f * PI - .001f ? -error :
			clampr(angle - error, lo, hi) - angle; // ODE first body relative to second.
		float velocity = clampr(remaining * 6.f, -m_max_gun_speed, m_max_gun_speed);
		if (_abs(remaining) < deg2rad(.1f))
			velocity = 0.f;
		joint->SetForceAndVelocity(torques[index], velocity, 0);
	}
	m_allow_fire = m_fire_dir.dotproduct(m_destEnemyDir) >= cosf(deg2rad(5.f));
}
const Fvector&	CCarWeapon::ViewCameraPos()
{
	return m_fire_pos;
}

void CCarWeapon::ConfigureOperatorCamera(CCameraBase& camera, bool external)
{
	// COP_ENGINE_PATCH: COP-ENG-203 - camera aim is independent of servo lag.
	// The inactive turret may have travelled/turned with the chassis since use.
	IKinematics* kinematics = smart_cast<IKinematics*>(m_object->Visual());
	kinematics->CalculateBones_Invalidate();
	kinematics->CalculateBones(TRUE);
	UpdateFireTransform();
	Fmatrix inverse;
	inverse.invert(m_object->XFORM());
	Fvector local;
	inverse.transform_dir(local, m_fire_dir);
	camera.Set(-local.getH(), -local.getP(), 0.f);
	camera.vDirection.set(m_fire_dir);
	camera.vNormal.set(m_fire_norm);
	camera.vPosition.set(OperatorCameraPosition(external));
	camera.bClampYaw = FALSE;
	camera.bClampPitch = TRUE;
	if (external)
	{
		camera.lim_pitch.set(-deg2rad(80.f), deg2rad(80.f));
		static_cast<CCameraLook&>(camera).ConfigureDistance(m_operator_external_zoom);
	}
	else camera.lim_pitch.set(-m_lim_x_rot.y, -m_lim_x_rot.x);
}

Fvector CCarWeapon::OperatorCameraPosition(bool external) const
{
	// COP_ENGINE_PATCH: COP-ENG-205 - the camera can follow a yaw-only turret
	// anchor; raising the barrel must not carry the eye through the breech.
	IKinematics* kinematics = smart_cast<IKinematics*>(m_object->Visual());
	Fmatrix anchor;
	anchor.mul_43(m_object->XFORM(), kinematics->LL_GetTransform(m_camera_bone));
	Fvector position;
	anchor.transform_tiny(position, external ? m_operator_external_offset : m_operator_camera_offset);
	return position;
}

const Fvector&	CCarWeapon::ViewCameraDir()
{
	return m_fire_dir;
}

const Fvector&	CCarWeapon::ViewCameraNorm()
{
	return m_fire_norm;
}

// COP_ENGINE_PATCH: COP-ENG-206 - only the actor occupying this car's operator seat.
CActor* CCarWeapon::AmmoUser() const
{
	CCar* car = smart_cast<CCar*>(m_object);
	CActor* actor = car ? car->WeaponOwner() : nullptr;
	return m_bActive && actor && actor->g_Alive() ? actor : nullptr;
}

bool CCarWeapon::TakeCartridge()
{
	CActor* actor = AmmoUser();
	if (!actor) return false;
	// Match the ordinary weapon's explicit unlimited-ammo cheat, including the
	// cartridge's opt-out. With the cheat disabled every bullet costs one round.
	if (actor->unlimited_ammo() && m_Ammo->m_flags.test(CCartridge::cfCanBeUnlimited))
		return true;
	PIItem item = actor->inventory().GetAny(m_inventory_ammo_section.c_str());
	CWeaponAmmo* ammo = item ? item->cast_weapon_ammo() : nullptr;
	CCartridge cartridge = *m_Ammo;
	if (!ammo || !ammo->Get(cartridge)) return false;
	if (!ammo->m_boxCurr && OnServer())
		ammo->SetDropManual(TRUE); // same deferred empty-box cleanup as CWeaponMagazined.
	return true;
}

u32 CCarWeapon::InventoryAmmoCount() const
{
	CActor* actor = AmmoUser();
	if (!m_inventory_ammo || !actor) return 0;
	u32 count = 0;
	for (PIItem item : actor->inventory().m_all)
		if (CWeaponAmmo* ammo = item->cast_weapon_ammo())
			if (ammo->cNameSect() == m_inventory_ammo_section) count += ammo->m_boxCurr;
	return count;
}

LPCSTR CCarWeapon::InventoryAmmoName() const
{
	return m_Ammo->GetInventoryName();
}
