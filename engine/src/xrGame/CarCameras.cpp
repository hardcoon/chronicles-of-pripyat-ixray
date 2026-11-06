#include "StdAfx.h"

#ifdef DEBUG

#include "PHDebug.h"
#include "../xrPhysics/IPHWorld.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "Actor.h"
#include "CarWeapon.h"
#include "cameralook.h"
#include "CameraFirstEye.h"
#include "Level.h"
#include "../xrEngine/CameraManager.h"
#include "../xrSound/Sound.h"

namespace
{
float CameraPersistence(float dt, float response)
{
	return expf(-dt / _max(response, EPS_S));
}

void RotateCameraBasis(CCameraBase* camera, Fvector axis, float angle)
{
	if (_abs(angle) <= EPS_S || axis.square_magnitude() <= EPS_S)
		return;

	axis.normalize_safe();
	Fquaternion rotation_quaternion;
	rotation_quaternion.rotation(axis, angle);
	Fmatrix rotation;
	rotation.rotation(rotation_quaternion);
	rotation.transform_dir(camera->vDirection);
	rotation.transform_dir(camera->vNormal);

	camera->vDirection.normalize_safe();
	Fvector right;
	right.crossproduct(camera->vNormal, camera->vDirection);
	right.normalize_safe();
	camera->vNormal.crossproduct(camera->vDirection, right);
	camera->vNormal.normalize_safe();
}
}

bool CCar::HUDView() const
{
	return active_camera->tag==ectFirst;
}

void CCar::ResetVehicleCameraDynamics()
{
	m_camera_previous_local_velocity.set(0.f, 0.f, 0.f);
	m_camera_filtered_local_acceleration.set(0.f, 0.f, 0.f);
	m_camera_local_offset.set(0.f, 0.f, 0.f);
	m_camera_local_angles.set(0.f, 0.f, 0.f);
	m_camera_dynamics_valid = false;
}

void CCar::UpdateVehicleCameraDynamics(float dt)
{
	if (!m_vehicle_camera_dynamics || !m_pPhysicsShell || dt <= EPS_S || dt > 0.25f)
	{
		ResetVehicleCameraDynamics();
		return;
	}

	Fvector world_velocity;
	m_pPhysicsShell->get_LinearVel(world_velocity);
	Fvector local_velocity;
	local_velocity.set(
		world_velocity.dotproduct(XFORM().i),
		world_velocity.dotproduct(XFORM().j),
		world_velocity.dotproduct(XFORM().k));

	if (!m_camera_dynamics_valid)
	{
		m_camera_previous_local_velocity = local_velocity;
		m_camera_dynamics_valid = true;
		return;
	}

	Fvector local_acceleration;
	local_acceleration.sub(local_velocity, m_camera_previous_local_velocity).div(dt);
	m_camera_previous_local_velocity = local_velocity;
	clamp(local_acceleration.x, -m_camera_max_lateral_acceleration, m_camera_max_lateral_acceleration);
	local_acceleration.y = 0.f;
	clamp(local_acceleration.z, -m_camera_max_longitudinal_acceleration, m_camera_max_longitudinal_acceleration);
	m_camera_filtered_local_acceleration.inertion(
		local_acceleration, CameraPersistence(dt, m_camera_acceleration_response));

	Fvector target_offset;
	target_offset.set(
		-m_camera_filtered_local_acceleration.x * m_camera_lateral_offset_per_accel,
		0.f,
		-m_camera_filtered_local_acceleration.z * m_camera_longitudinal_offset_per_accel);
	target_offset.mul(m_camera_motion_strength);
	clamp(target_offset.x, -m_camera_max_lateral_offset * m_camera_motion_strength,
		m_camera_max_lateral_offset * m_camera_motion_strength);
	clamp(target_offset.z, -m_camera_max_longitudinal_offset * m_camera_motion_strength,
		m_camera_max_longitudinal_offset * m_camera_motion_strength);

	Fvector target_angles;
	// COP_ENGINE_PATCH: COP-ENG-053 - lateral acceleration may move the head a
	// little inside the cabin, but it must never roll the horizon. Otherwise a
	// steering input adds artificial banking on top of chassis stabilization.
	target_angles.set(
		-m_camera_filtered_local_acceleration.z * m_camera_pitch_per_accel,
		0.f,
		0.f);
	target_angles.mul(m_camera_motion_strength);
	clamp(target_angles.x, -m_camera_max_pitch * m_camera_motion_strength,
		m_camera_max_pitch * m_camera_motion_strength);

	const bool returning = m_camera_filtered_local_acceleration.square_magnitude() < 0.01f;
	const float persistence = CameraPersistence(dt, returning ? m_camera_motion_return : m_camera_motion_response);
	m_camera_local_offset.inertion(target_offset, persistence);
	m_camera_local_angles.inertion(target_angles, persistence);
}

void CCar::ApplyVehicleCameraHorizon()
{
	// COP_ENGINE_PATCH: COP-ENG-104 - the comfort horizon belongs to the
	// first-person cockpit camera only. Third-person chase/free cameras already
	// use their own world/vehicle reference and must not inherit rollover tilt.
	if (!m_vehicle_camera_dynamics || !active_camera || active_camera->tag != ectFirst)
		return;

	const Fvector world_up = Fvector().set(0.f, 1.f, 0.f);
	if (active_camera->m_Flags.is(CCameraBase::flRelativeLink))
	{
		// COP_ENGINE_PATCH: COP-ENG-053 - correct roll in the camera's own view
		// plane. The old chassis-axis correction was valid only while looking
		// forward; yawing left or right converted it into visible camera banking.
		Fvector view_direction = active_camera->vDirection;
		view_direction.normalize_safe();
		Fvector horizon_up = world_up;
		Fvector vertical_component = view_direction;
		vertical_component.mul(world_up.dotproduct(view_direction));
		horizon_up.sub(vertical_component);
		if (horizon_up.square_magnitude() <= EPS_S)
			return;
		horizon_up.normalize_safe();

		Fvector camera_up = active_camera->vNormal;
		camera_up.normalize_safe();
		Fvector roll_cross;
		roll_cross.crossproduct(camera_up, horizon_up);
		const float roll_error = atan2f(view_direction.dotproduct(roll_cross),
			clampr(camera_up.dotproduct(horizon_up), -1.f, 1.f));
		// COP_ENGINE_PATCH: COP-ENG-054 - a small chassis roll must produce an
		// exactly level horizon, not merely rotate toward it. For larger rolls
		// keep only the part beyond the comfort limit. X-Ray's row-vector
		// transform_dir convention applies a quaternion angle with the opposite
		// sign, hence the minus on the capped correction below.
		if (_abs(roll_error) <= m_camera_horizon_limit)
		{
			active_camera->vNormal = horizon_up;
			Fvector right;
			right.crossproduct(active_camera->vNormal, view_direction);
			right.normalize_safe();
			active_camera->vNormal.crossproduct(view_direction, right);
			active_camera->vNormal.normalize_safe();
			return;
		}
		const float correction = clampr(roll_error, -m_camera_horizon_limit, m_camera_horizon_limit);
		RotateCameraBasis(active_camera, view_direction, -correction);
		return;
	}

	Fvector car_up = XFORM().j;
	car_up.normalize_safe();
	float up_dot = car_up.dotproduct(world_up);
	clamp(up_dot, -1.f, 1.f);
	const float tilt = acosf(up_dot);
	if (tilt <= EPS_S)
		return;

	float correction = _min(tilt, m_camera_horizon_limit);
	const float soft_zone = _min(deg2rad(3.f), m_camera_horizon_limit);
	if (soft_zone > EPS_S && tilt > m_camera_horizon_limit - soft_zone &&
		tilt < m_camera_horizon_limit + soft_zone)
	{
		float blend = (tilt - (m_camera_horizon_limit - soft_zone)) / (2.f * soft_zone);
		blend = blend * blend * (3.f - 2.f * blend);
		correction = tilt + (m_camera_horizon_limit - tilt) * blend;
	}

	Fvector correction_axis;
	correction_axis.crossproduct(car_up, world_up);
	if (correction_axis.square_magnitude() <= EPS_S)
	{
		correction_axis = XFORM().k;
		correction_axis.y = 0.f;
		if (correction_axis.square_magnitude() <= EPS_S)
		{
			correction_axis = XFORM().i;
			correction_axis.y = 0.f;
		}
	}
	correction_axis.normalize_safe();

	// The free camera starts horizon-relative. Add only the chassis tilt that
	// exceeds the comfort envelope, so a rollover still turns the view over.
	correction_axis.invert();
	RotateCameraBasis(active_camera, correction_axis, tilt - correction);
}

void CCar::ApplyVehicleCameraMotion()
{
	if (!m_vehicle_camera_dynamics || !active_camera || active_camera->tag != ectFirst)
		return;

	Fvector right = active_camera->Right();
	RotateCameraBasis(active_camera, right, m_camera_local_angles.x);
}

void	CCar::cam_Update			(float dt, float fov)
{
	VERIFY(!physics_world()->Processing());
	Fvector							P,Da;
	Da.set							(0,0,0);
	//bool							owner = !!Owner();

	const bool gunner = IsWeaponMode();
	if (!gunner) UpdateVehicleCameraDynamics(dt);
	Fvector local_camera_position = *m_camera_current_position;
	if (m_vehicle_camera_dynamics && active_camera && active_camera->tag == ectFirst)
		local_camera_position.add(m_camera_local_offset);
	XFORM().transform_tiny			(P, local_camera_position);
	// COP_ENGINE_PATCH: COP-ENG-203 - position follows the current weapon bone,
	// while orientation stays under native camera/mouse control.
	// COP_ENGINE_PATCH: COP-ENG-205 - first-person eye and external orbit use their own anchors.
	if (gunner) P = m_car_weapon->OperatorCameraPosition(IsWeaponExternalView());
	// COP_ENGINE_PATCH: COP-ENG-040 - never low-pass a seat position in world
	// space. That makes the camera lag behind a moving chassis and then pass
	// through the seat. DCP camera constants are not IX-Ray world-position
	// interpolation coefficients. COP-ENG-052 adds only a bounded local offset,
	// so the camera anchor remains attached to the configured seat point.

	switch(active_camera->tag) {
	case ectFirst:
		// rotate head
		if(OwnerActor()) OwnerActor()->Orientation().yaw	= -active_camera->yaw;
		if(OwnerActor()) OwnerActor()->Orientation().pitch	= -active_camera->pitch;
		break;
	case ectChase:						break;
	case ectFree:						break;
	}
	active_camera->f_fov				= fov;
	active_camera->Update				(P,Da);
	if (!gunner)
	{
		ApplyVehicleCameraHorizon();
		ApplyVehicleCameraMotion();
	}
	Level().Cameras().UpdateFromCamera	(active_camera);
}

void	CCar::OnCameraChange		(int type)
{
	if (m_dcp_white_niva_profile)
	{
		switch (type)
		{
		case ectFirst: m_camera_current_position = &m_camera_position; break;
		case ectChase: m_camera_current_position = &m_camera_position_2; break;
		case ectFree: m_camera_current_position = &m_camera_position_3; break;
		default: NODEFAULT;
		}
	}
	ResetVehicleCameraDynamics();
	if(Owner())
	{
		if	(type==ectFirst)
		{
			if (!m_dcp_white_niva_profile)
				m_camera_current_position = &m_camera_position;
			Owner()->setVisible(FALSE);
		}
		else if(active_camera->tag==ectFirst)
		{
			if (!m_dcp_white_niva_profile)
				m_camera_current_position = &m_camera_position_2;
			Owner()->setVisible(TRUE);
		}
	}
	
	if (!active_camera||active_camera->tag!=type){
		active_camera	= camera[type];
		if (ectFree==type){
			Fvector xyz;
			XFORM().getXYZi(xyz);
			active_camera->yaw		= xyz.y;
		}
	}

	// COP_ENGINE_PATCH: COP-ENG-056 - camera changes no longer alter the global
	// effects bus. SCarSound reads the active camera and applies interior_volume
	// only to this vehicle's own sources.
}

