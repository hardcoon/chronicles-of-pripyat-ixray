#pragma once

#include "../xrEngine/CameraBase.h"

class CCameraLook	: public CCameraBase
{
	typedef CCameraBase inherited;

protected:
	Fvector2		lim_zoom;
	float			dist, prev_d;

public:
					CCameraLook		( CObject* p, u32 flags=0);
	virtual			~CCameraLook	( );
	virtual void	Load			(LPCSTR section);
	virtual void	Move			( int cmd, float val=0, float factor=1.0f );

	virtual	void	OnActivate		( CCameraBase* old_cam );
	virtual void	Update			( Fvector& point, Fvector& noise_dangle );

	virtual float	GetWorldYaw		( )	{ return -yaw;	};
	virtual float	GetWorldPitch	( )	{ return pitch; };
	virtual float	GetDist		( )	{ return prev_d;	};	
	// COP_ENGINE_PATCH: COP-ENG-205 - used only by the separate vehicle operator camera.
	void ConfigureDistance(const Fvector2& limits)
	{
		lim_zoom.set(_max(limits.x, 1.f), _max(limits.y, _max(limits.x, 1.f)));
		dist = (lim_zoom.x + lim_zoom.y) * .5f;
		prev_d = dist;
	}
	void save(NET_Packet& output_packet) override;
	void load(IReader& input_packet) override;

protected:
			void	UpdateDistance	( Fvector& point );
};

class CCameraLook2	: public CCameraLook
{
public:
	static Fvector	m_cam_offset_r;
	static Fvector	m_cam_offset_l;
	static bool		m_use_inertion;

	CCameraLook2(CObject* p, u32 flags = 0) : CCameraLook(p, flags) {};

	virtual			~CCameraLook2	(){}
	virtual	void	OnActivate		( CCameraBase* old_cam );
	virtual void	Update			( Fvector& point, Fvector& noise_dangle );
	virtual void	Load			(LPCSTR section);
protected:
	void	UpdateDistance(Fvector& pivot, Fvector& correction);
};

class CCameraFixedLook : public CCameraLook
{
	typedef CCameraLook inherited;
public:
					CCameraFixedLook(CObject* p, u32 flags=0) : CCameraLook(p, flags) {};
	virtual			~CCameraFixedLook() {};
	virtual void	Load			(LPCSTR section);
	virtual void	Move			(int cmd, float val=0, float factor=1.0f);
	virtual	void	OnActivate		(CCameraBase* old_cam);
	virtual void	Update			(Fvector& point, Fvector& noise_dangle);
	virtual void	Set				(float Y, float P, float R);
private:
	Fquaternion		m_final_dir;
	Fquaternion		m_current_dir;
};

