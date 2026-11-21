#ifndef  PHNETSTATE_H
#define  PHNETSTATE_H

#include "../xrEngine/VisMask.h"

class NET_Packet;

struct SPHNetState
{
	Fvector linear_vel = {};
	Fvector angular_vel = {};
	Fvector force = {};
	Fvector torque = {};
	Fvector position = {};
	Fvector previous_position = {};

	union
	{
		Fquaternion quaternion = {};
		struct
		{
			Fvector	accel;
			float	max_velocity;
		};
	};

	Fquaternion	previous_quaternion = {};

	bool enabled = false;

	void net_Export(NET_Packet& P);
	void net_Import(NET_Packet& P);
	void net_Import(IReader& P);
	// COP_ENGINE_PATCH: COP-ENG-236 - native-float save poses, no quantization.
	void save_pose(NET_Packet& P);
	void load_pose(NET_Packet& P);
	void load_pose(IReader& P);
	void net_Save(NET_Packet& P);
	void net_Load(NET_Packet& P);
	void net_Load(IReader& P);
	void net_Save(NET_Packet& P, const Fvector& min, const Fvector& max);
	void net_Load(NET_Packet& P, const Fvector& min, const Fvector& max);
	void net_Load(IReader& P, const Fvector& min, const Fvector& max);
private:

	template<typename src>
	void read(src& P);
	template<typename src>
	void read_pose(src& P);
	template<typename src>
	void read(src& P, const Fvector& min, const Fvector& max);
};

using PHNETSTATE_VECTOR = xr_vector<SPHNetState>; 
using PHNETSTATE_I = PHNETSTATE_VECTOR::iterator;

struct SPHBonesData
{
	// Old saves have an untagged count and 8-bit poses. The high bit is
	// reserved for native-float poses; it is never part of the element count.
	static constexpr u16 full_precision_flag = 0x8000;
	VisMask			  bones_mask;
	u16				  root_bone;
	PHNETSTATE_VECTOR bones;
	Fvector			  m_min;
	Fvector			  m_max;
public:
	SPHBonesData		()						  ;
	void								net_Save			(		NET_Packet&		P);					
	void								net_Load			(		NET_Packet&		P);
	void								net_Load			(		IReader&		P);
	void								set_min_max			(const Fvector& _min, const Fvector& _max);
	const Fvector&						get_min				()	const	{return m_min;}
	const Fvector&						get_max				()	const	{return m_max;}
};
#endif