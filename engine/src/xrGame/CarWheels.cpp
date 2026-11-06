#include "StdAfx.h"
#ifdef DEBUG

#include "PHDebug.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xrPhysics/ExtendedGeom.h"
#include "../xrEngine/GameMtlLib.h"

CCar::SWheel::SWheelCollisionParams::SWheelCollisionParams()
{
		spring_factor								=1		;
		damping_factor								=1		;
		mu_factor									=1		;
		dynamic_mu_factor							=1		;
		dynamic_mu2_factor						=1		;
		friction2_factor							=1		;
		slip_speed								=0		;
		friction_direction.set					(1.f, 0.f, 0.f);
		dcp_profile								=false	;
		steering									=false	;
		handbrake_wheel							=false	;
		spinning									=false	;
		surface										=dcpSurfaceRoad;
		last_contact_time							=0;
}
IC void CCar::SWheel::applywheelCollisionParams(const dxGeomUserData *ud,bool& do_colide,dContact& c,SGameMtl* material_1,SGameMtl* material_2)
{
	if(ud&&ud->object_callbacks&&ud->object_callbacks->HasCallback(WheellCollisionCallback))
	{
		SWheelCollisionParams	&cp	=		*((SWheelCollisionParams*)(ud->callback_data))	;
		dSurfaceParameters		&sp	=		c.surface										;
		sp.mu						*=		cp.mu_factor									;
		if (cp.dcp_profile)
		{
			// COP_ENGINE_PATCH: COP-ENG-040 - anisotropic tyre coefficients must
			// follow the wheel's rolling direction. Without dContactFDir1 ODE
			// chooses an arbitrary tangent, so longitudinal and lateral grip swap
			// unpredictably and the car can neither launch nor steer reliably.
			Fvector normal;
			normal.set((float)c.geom.normal[0], (float)c.geom.normal[1], (float)c.geom.normal[2]);
			Fvector rolling_direction;
			rolling_direction.crossproduct(normal, cp.friction_direction);
			if (rolling_direction.square_magnitude() > EPS_S)
			{
				rolling_direction.normalize();
				sp.mode |= dContactFDir1 | dContactMu2;
				c.fdir1[0] = rolling_direction.x;
				c.fdir1[1] = rolling_direction.y;
				c.fdir1[2] = rolling_direction.z;
				c.fdir1[3] = 0;
				sp.mu2 = sp.mu * cp.friction2_factor * cp.dynamic_mu2_factor;
			}
			sp.mu *= cp.dynamic_mu_factor;
			if (cp.slip_speed > EPS)
			{
				sp.mode |= dContactSlip1 | dContactSlip2;
				sp.slip1 = cp.slip_speed;
				sp.slip2 = cp.slip_speed;
			}
		}
		MulSprDmp(sp.soft_cfm,sp.soft_cfm,cp.spring_factor,cp.damping_factor)				;
		if (material_1)
		{
			if (material_1->Flags.test(SGameMtl::flLiquid))
			{
				cp.surface = dcpSurfaceWater;
			}
			else
			{
				string256 material_name;
				xr_strcpy(material_name, *material_1->m_Name);
				xr_strlwr(material_name);
				if (strstr(material_name, "grass") || strstr(material_name, "bush") || strstr(material_name, "plant"))
					cp.surface = dcpSurfaceGrass;
				else if (strstr(material_name, "earth") || strstr(material_name, "ground") ||
					strstr(material_name, "dirt") || strstr(material_name, "mud") || strstr(material_name, "sand"))
					cp.surface = dcpSurfaceDirt;
				else
					cp.surface = dcpSurfaceRoad;
			}
			cp.last_contact_time = Device.dwTimeGlobal;
		}
	}
}

void  CCar::SWheel::WheellCollisionCallback(bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2)
{
	
	dxGeomUserData					*ud1			=			PHRetrieveGeomUserData(c.geom.g1)	;
	dxGeomUserData					*ud2			=			PHRetrieveGeomUserData(c.geom.g2)	;
	applywheelCollisionParams		(ud1,do_colide,c,material_2,nullptr)						;
	applywheelCollisionParams		(ud2,do_colide,c,material_1,nullptr)						;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CCar::WheelHit(float P,s16 element,ALife::EHitType hit_type)
{
	xr_map   <u16,SWheel>::iterator i=m_wheels_map.find(element);
	if(i!=m_wheels_map.end())
	{
		i->second.Hit(P);
		return true;
	}
	else return false;
}
void CCar::SWheel::Init()
{
	if(inited) return;
	BONE_P_PAIR_CIT bone= car->bone_map.find(bone_id);
	R_ASSERT2(bone->second.element,"No Element was created for wheel. Check collision is set");
	bone->second.element->set_DynamicLimits(default_l_limit,default_w_limit*100.f);
	CPhysicsElement	*e=bone->second.element	;
	CPhysicsJoint	*j=bone->second.joint	;
	radius=e->getRadius();
	R_ASSERT2(j,"No wheel joint was set for a wheel");
	joint=j;
	joint->SetBackRef(&joint);

	//R_ASSERT2(dJointGetType(joint->GetDJoint())==dJointTypeHinge2,"No wheel join was set for a wheel, only wheel-joint valid!!!");
	R_ASSERT2(joint->IsWheelJoint(),"No wheel join was set for a wheel, only wheel-joint valid!!!");

	ApplyDriveAxisVelTorque(0.f,0.f);
	e->add_ObjectContactCallback(WheellCollisionCallback);
	e->set_CallbackData((void*)&collision_params);
	e->SetAirResistance(0,0);
	if (car->m_dcp_white_niva_profile)
		e->set_ApplyByGravity(false);
	inited=true;
}
void CCar::SWheel::Load(LPCSTR section)
{
	IKinematics		*K			=PKinematics(car->Visual())		;
	CInifile		*ini		=K->LL_UserData()				;
	VERIFY						(ini)							;
	if(ini->section_exist(section))
	{
		collision_params.damping_factor	=READ_IF_EXISTS(ini,r_float,section,"damping_factor",collision_params.damping_factor);
		collision_params.spring_factor	=READ_IF_EXISTS(ini,r_float,section,"spring_factor",collision_params.spring_factor);
		collision_params.mu_factor		=READ_IF_EXISTS(ini,r_float,section,"friction_factor",collision_params.mu_factor);
	} 
	else if(ini->section_exist("wheels_params"))
	{
		collision_params.damping_factor	=ini->r_float("wheels_params","damping_factor")		;	
		collision_params.spring_factor	=ini->r_float("wheels_params","spring_factor")		;
		collision_params.mu_factor		=ini->r_float("wheels_params","friction_factor")	;
	}
	if (car->m_dcp_white_niva_profile && ini->section_exist("wheels_params"))
	{
		collision_params.dcp_profile = true;
		// COP_ENGINE_PATCH: COP-ENG-043 - donor veh_niva_u_02 does not enable
		// ODE contact-slip bits. Apply the translation scale before the first
		// contact as well as during PhDataUpdate, so there is no one-frame slip.
		collision_params.slip_speed = car->m_friction_slip_speed * car->m_contact_slip_scale;
		suspension_distance = READ_IF_EXISTS(ini, r_float, "wheels_params", "suspension_distance", suspension_distance);
		suspension_force_inc = READ_IF_EXISTS(ini, r_float, "wheels_params", "suspension_force_inc", suspension_force_inc);
		suspension_spring_factor = READ_IF_EXISTS(ini, r_float, "wheels_params", "suspension_spring_factor", suspension_spring_factor);
		suspension_damping_factor = READ_IF_EXISTS(ini, r_float, "wheels_params", "suspension_damping_factor", suspension_damping_factor);
		suspension_inertia = READ_IF_EXISTS(ini, r_float, "wheels_params", "suspension_inertia", suspension_inertia);
	}

}
void CCar::SWheel::ApplyDriveAxisTorque(float torque)
{
	if(!joint) return;
	//dJointSetHinge2Param(joint->GetDJoint(), dParamFMax2,torque);//car->m_axle_friction
	joint->SetForce( torque, 1 );
}
void CCar::SWheel::ApplyDriveAxisVel(float vel)
{
	if(!joint) return;
	//dJointSetHinge2Param(joint->GetDJoint(), dParamVel2, vel);
	joint->SetVelocity( vel, 1 );
}


void CCar::SWheel::ApplyDriveAxisVelTorque(float vel,float torque)
{
	ApplyDriveAxisVel(vel);
	ApplyDriveAxisTorque(torque);

}
void CCar::SWheel::ApplySteerAxisVel(float vel)
{
	if(!joint) return;
	//dJointSetHinge2Param(joint->GetDJoint(), dParamVel, vel);
	joint->SetVelocity( vel, 0 );
}

void CCar::SWheel::ApplySteerAxisTorque(float torque)
{
	if(!joint) return;
	//dJointSetHinge2Param(joint->GetDJoint(), dParamFMax, torque);
	joint->SetForce( torque, 0 );
}

void CCar::SWheel::ApplySteerAxisVelTorque(float vel,float torque)
{
	ApplySteerAxisVel(vel);
	ApplySteerAxisTorque(torque);
}

void CCar::SWheel::SetSteerHiLimit(float hi)
{
	if(!joint) return;
	//dJointSetHinge2Param(joint->GetDJoint(), dParamHiStop, hi);
	joint->SetHiLimitDynamic( 0, hi );
}
void CCar::SWheel::SetSteerLoLimit(float lo)
{
	if(!joint) return;
	//dJointSetHinge2Param(joint->GetDJoint(), dParamLoStop, lo);
	joint->SetLoLimitDynamic( 0, lo );
}
void CCar::SWheel::SetSteerLimits(float hi,float lo)
{
	SetSteerHiLimit(hi);
	SetSteerLoLimit(lo);
}

void CCar::SWheel::ApplyDamage(u16 level)
{
	inherited::ApplyDamage(level);
	if (!joint)
		return;

	if (level == 0)
		return;

	float sf, df;
	switch (level)
	{
	case 1:
		joint->GetJointSDfactors(sf, df);
		sf /= 20.f; df *= 4.f;
		joint->SetJointSDfactors(sf, df);
		car->m_damage_particles.PlayWheel1(car, bone_id);
		break;
	case 2:
	{
		Fvector v;
		joint->GetAxisDirDynamic(1, v);

		v[0] += 0.1f; v[1] += 0.1f; v[2] += 0.1f;
		VERIFY(v.magnitude() > EPS_S);
		v.normalize();

		joint->SetAxisDir(v, 1);
		joint->GetJointSDfactors(sf, df);
		sf /= 30.f; df *= 8.f;
		joint->SetJointSDfactors(sf, df);
		car->m_damage_particles.PlayWheel2(car, bone_id);
		break;
	}
	default: NODEFAULT;
	}
}

void CCar::SWheel::SaveNetState(NET_Packet& P)
{
	CSE_ALifeCar::SWheelState ws;
	ws.health=Health();
	ws.write(P);
}

void CCar::SWheel::RestoreNetState(const CSE_ALifeCar::SWheelState& a_state)
{
	SetHealth(a_state.health);
	RestoreEffect();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCar::SWheelDrive::Init()
{
	pwheel->Init();
	gear_factor = pwheel->radius / pwheel->car->m_ref_radius;

	const Fmatrix& bone_data = smart_cast<IKinematics*>(pwheel->car->Visual())->LL_GetTransform(pwheel->bone_id);
	pos_fvd = bone_data.k.x > 0.f ? -1.f : 1.f;
}

void CCar::SWheelDrive::Load(LPCSTR section)
{
	torque_factor = 1.f;
	CInifile* ini = PKinematics(pwheel->car->Visual())->LL_UserData();
	if (pwheel->car->m_dcp_white_niva_profile && ini->section_exist(section))
		torque_factor = _max(READ_IF_EXISTS(ini, r_float, section, "torque_factor", 1.f), 0.f);
}

void CCar::SWheelDrive::Drive()
{
	float cur_speed=pwheel->car->RefWheelMaxSpeed()/gear_factor;
	pwheel->ApplyDriveAxisVel(pos_fvd*cur_speed);
}
void CCar::SWheelDrive::UpdatePower()
{
	pwheel->ApplyDriveAxisTorque(pwheel->car->RefWheelCurTorque() * torque_factor / gear_factor);
}
void CCar::SWheelDrive::Neutral()
{
	pwheel->ApplyDriveAxisVelTorque(0.f,pwheel->car->m_axle_friction);
}

float CCar::SWheelDrive::ASpeed() const
{
	CPhysicsJoint* J = pwheel->joint;
	if (!J)
		return 0.f;

	return (J->GetAxisAngleRate(1)) * pos_fvd;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCar::SWheelSteer::Init()
{
	IKinematics* pKinematics=smart_cast<IKinematics*>(pwheel->car->Visual());
	pwheel->Init();
	(pwheel->car->bone_map.find(pwheel->bone_id))->second.joint->GetLimits(lo_limit,hi_limit,0);
	
	auto bone_data = smart_cast<IKinematics*>(pwheel->car->Visual())->LL_GetTransform(pwheel->bone_id);
	pos_right = bone_data.i.y > 0.f ? -1.f : 1.f;

	float steering_torque_=pKinematics->LL_UserData()->r_float("car_definition","steering_torque");
	VERIFY( pwheel );
	pwheel->ApplySteerAxisTorque(steering_torque_);
	VERIFY( pwheel->joint );
	//dJointSetHinge2Param(pwheel->joint->GetDJoint(), dParamFudgeFactor, 0.005f/steering_torque);
	pwheel->joint->SetJointFudgefactorActive( 0.005f/steering_torque_ );
	pwheel->ApplySteerAxisVel(0.f);
	pwheel->collision_params.steering = true;
	pwheel->collision_params.friction2_factor = pwheel->car->m_friction2_steering_wheels;
	limited=false;
}

void CCar::SWheelSteer::SteerRight()
{
	limited=true;						//no need to limit wheels when steering
	if(pos_right>0)
	{
		pwheel->SetSteerHiLimit(hi_limit * pwheel->car->m_steering_limit_factor);
		const float velocity = pwheel->car->DcpSteeringVelocity(pwheel->joint->GetAxisAngle(0), 1.f);
		pwheel->ApplySteerAxisVel(velocity);
	}
	else
	{
		pwheel->SetSteerLoLimit(lo_limit * pwheel->car->m_steering_limit_factor);
		const float velocity = pwheel->car->DcpSteeringVelocity(pwheel->joint->GetAxisAngle(0), -1.f);
		pwheel->ApplySteerAxisVel(-velocity);
	}
}
void CCar::SWheelSteer::SteerLeft()
{

	limited=true;						//no need to limit wheels when steering
	if(pos_right<0)
	{
		pwheel->SetSteerHiLimit(hi_limit * pwheel->car->m_steering_limit_factor);
		const float velocity = pwheel->car->DcpSteeringVelocity(pwheel->joint->GetAxisAngle(0), 1.f);
		pwheel->ApplySteerAxisVel(velocity);
	}
	else
	{
		pwheel->SetSteerLoLimit(lo_limit * pwheel->car->m_steering_limit_factor);
		const float velocity = pwheel->car->DcpSteeringVelocity(pwheel->joint->GetAxisAngle(0), -1.f);
		pwheel->ApplySteerAxisVel(-velocity);
	}
}
void CCar::SWheelSteer::SteerIdle()
{
	limited=false;
	if(pwheel->car->e_state_steer==right)
	{
		if(pos_right<0)
		{
			pwheel->SetSteerHiLimit(0.f);
			pwheel->ApplySteerAxisVel( pwheel->car->m_steering_speed);
		}
		else
		{
			pwheel->SetSteerLoLimit(0.f);
			pwheel->ApplySteerAxisVel(-pwheel->car->m_steering_speed);
		}
	}
	else
	{
		if(pos_right>0)
		{
			pwheel->SetSteerHiLimit(0.f);
			pwheel->ApplySteerAxisVel( pwheel->car->m_steering_speed);
		}
		else
		{
			pwheel->SetSteerLoLimit(0.f);
			pwheel->ApplySteerAxisVel(-pwheel->car->m_steering_speed);
		}
	}

}

void CCar::SWheelSteer::Limit()
{
	CPhysicsJoint* J=pwheel->joint;
	if(!J) return;
	//dJointID joint=J->GetDJoint();
	if(!limited)
	{
		//dReal angle = dJointGetHinge2Angle1(joint);
		float angle = J->GetAxisAngle( 0 );
		if(_abs(angle)<M_PI/180.f)
		{
			pwheel->SetSteerLimits(0.f,0.f);
			pwheel->ApplySteerAxisVel(0.f);
			limited=true;
		}
	}
	pwheel->car->b_wheels_limited=pwheel->car->b_wheels_limited&&limited;
}


float  CCar::SWheelSteer::GetSteerAngle()
{
	VERIFY( pwheel );
	VERIFY( pwheel->joint );
	return -pos_right* pwheel->joint->GetAxisAngle( 0 );//dJointGetHinge2Angle1 (pwheel->joint->GetDJoint());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCar::SWheelBreak::Init()
{
	pwheel->Init();
	float k=pwheel->radius/pwheel->car->m_ref_radius;

	break_torque			*=k;
	hand_break_torque		*=k;
	pwheel->collision_params.handbrake_wheel = hand_break_torque > EPS;
	if (!pwheel->collision_params.steering)
		pwheel->collision_params.friction2_factor = pwheel->car->m_friction2_nonsteering_wheels;
}
void CCar::SWheelBreak::Load(LPCSTR section)
{
	IKinematics		*K			=PKinematics(pwheel->car->Visual())												;
	CInifile		*ini		=K->LL_UserData()																;
	VERIFY						(ini)																			;
	break_torque		=		ini->r_float("car_definition","break_torque")									;
	hand_break_torque	=		READ_IF_EXISTS(ini,r_float,"car_definition","hand_break_torque",break_torque)	;
	if(ini->section_exist(section))
	{	
		break_torque					=READ_IF_EXISTS(ini,r_float,section,"break_torque",break_torque);
		hand_break_torque				=READ_IF_EXISTS(ini,r_float,section,"hand_break_torque",hand_break_torque);
	}
}
void CCar::SWheelBreak::Break(float k)
{
	const float boost = pwheel->car->m_dcp_white_niva_profile ? pwheel->car->m_brake_boost : 1.f;
	pwheel->ApplyDriveAxisVelTorque(0.f,100000.f*break_torque*k*boost);
}

void CCar::SWheelBreak::HandBreak()
{
	pwheel->ApplyDriveAxisVelTorque(0.f,100000.f*hand_break_torque);
}

void CCar::SWheelBreak::Neutral()
{
	pwheel->ApplyDriveAxisVelTorque(0.f,pwheel->car->m_axle_friction);
}

