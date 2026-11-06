#include "StdAfx.h"
#include "MosquitoBald.h"
#include "Level.h"
#include "PhysicsShellHolder.h"
#include "../xrEngine/xr_collide_form.h"

CMosquitoBald::CMosquitoBald(void) 
{
	m_fHitImpulseScale		= 1.f;
	m_bLastBlowoutUpdate	= false;
	m_fCarHitScale			= 1.f;
	m_fCarHitKoef			= 1.f;
	m_fCarHitImpulseScale	= 1.f;
}

CMosquitoBald::~CMosquitoBald(void) 
{
}

void CMosquitoBald::Load(LPCSTR section)
{
	inherited::Load(section);
	m_fCarHitScale = READ_IF_EXISTS(pSettings, r_float, section, "car_hit_scale", 1.f);
	m_fCarHitKoef = READ_IF_EXISTS(pSettings, r_float, section, "hit_koef_car", 1.f);
	m_fCarHitImpulseScale = READ_IF_EXISTS(
		pSettings, r_float, section, "car_hit_impulse_scale", m_fHitImpulseScale);
}


bool CMosquitoBald::BlowoutState()
{
	bool result = inherited::BlowoutState();
	if(!result)
	{
		m_bLastBlowoutUpdate = false;
		UpdateBlowout();
	}
	else if(!m_bLastBlowoutUpdate)
	{
		m_bLastBlowoutUpdate = true;
		UpdateBlowout();
	}

	return result;
}
//bool CMosquitoBald::SecondaryHitState()
//{
//	bool result = inherited::SecondaryHitState();
//	if(!result)
//		UpdateBlowout();
//
//	return result;
//}

void CMosquitoBald::Affect(SZoneObjectInfo* O) 
{
	CPhysicsShellHolder *pGameObject = smart_cast<CPhysicsShellHolder*>(O->object);
	if(!pGameObject) return;

	if(O->zone_ignore) return;

	Fvector P; 
	XFORM().transform_tiny(P,CFORM()->getSphere().P);

	Fvector hit_dir; 
	hit_dir.set(	::Random.randF(-.5f,.5f), 
					::Random.randF(.0f,1.f), 
					::Random.randF(-.5f,.5f)); 
	hit_dir.normalize();

	Fvector position_in_bone_space;

	VERIFY(!pGameObject->getDestroy());

	float dist = pGameObject->Position().distance_to(P) - pGameObject->Radius();
	float power = Power(dist>0.f?dist:0.f, Radius());
	float impulse = m_fHitImpulseScale*power*pGameObject->GetMass();

	if (IsProfiledVehicle(pGameObject))
	{
		// COP_ENGINE_PATCH: COP-ENG-066 - donor Springboard formula for cars:
		// (0.25 * mass + 1000) * car_hit_scale. The named profile fades the
		// complete response to zero for heavy vehicles.
		const float response = VehicleAnomalyResponse(pGameObject);
		if (response <= EPS)
			return;

		Fvector vehicle_dir;
		vehicle_dir.sub(pGameObject->Position(), P);
		if (vehicle_dir.square_magnitude() > EPS_S)
		{
			vehicle_dir.normalize();
			const float vehicle_impulse =
				(pGameObject->GetMass() * 0.25f + 1000.f) * m_fCarHitScale * response;
			pGameObject->PPhysicsShell()->applyImpulse(vehicle_dir, vehicle_impulse);
		}

		impulse = m_fCarHitImpulseScale * power * pGameObject->GetMass() * response;
		power *= m_fCarHitKoef * response * VehicleAnomalyDamageScale(pGameObject);
	}

	if(power > 0.01f)
	{
		position_in_bone_space.set(0.f,0.f,0.f);
		CreateHit(pGameObject->ID(),ID(),hit_dir,power,0,position_in_bone_space,impulse,m_eHitTypeBlowout);
		PlayHitParticles(pGameObject);
	}
}

void CMosquitoBald::UpdateSecondaryHit()
{
	if(m_dwAffectFrameNum == Device.dwFrame)	
		return;

	m_dwAffectFrameNum	= Device.dwFrame;
	if(Device.dwPrecacheFrame)					
		return;

	for (SZoneObjectInfo& info : m_ObjectInfoMap)
	{
		if(info.object && !info.object->getDestroy())
		{
			CPhysicsShellHolder *pGameObject = info.object->cast_physics_shell_holder();
			if(!pGameObject) return;

			if(info.zone_ignore) return;
			Fvector P; 
			XFORM().transform_tiny(P,CFORM()->getSphere().P);

			Fvector hit_dir; 
			hit_dir.set(	::Random.randF(-.5f,.5f), 
							::Random.randF(.0f,1.f), 
							::Random.randF(-.5f,.5f)); 
			hit_dir.normalize();

			Fvector position_in_bone_space;

			VERIFY(!pGameObject->getDestroy());

			float dist = pGameObject->Position().distance_to(P) - pGameObject->Radius();
			float power = m_fSecondaryHitPower * RelativePower(dist>0.f?dist:0.f, Radius());
			if(power<0.0f)
				return;

			float impulse = m_fHitImpulseScale*power*pGameObject->GetMass();
			if (IsProfiledVehicle(pGameObject))
			{
				// COP_ENGINE_PATCH: COP-ENG-068 - secondary damage uses the same
				// profile gate without weakening its physical impulse.
				const float response = VehicleAnomalyResponse(pGameObject);
				if (response <= EPS)
					continue;
				impulse *= response;
				power *= response * VehicleAnomalyDamageScale(pGameObject);
			}
			position_in_bone_space.set(0.f,0.f,0.f);
			CreateHit(pGameObject->ID(),ID(),hit_dir,power,0,position_in_bone_space,impulse,m_eHitTypeBlowout);
		}
	}
}
