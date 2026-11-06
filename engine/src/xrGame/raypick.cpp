#include "StdAfx.h"
#include "raypick.h"
#include "Level.h"

CRayPick::CRayPick() 
{
	start_position.set(0,0,0); 
	direction.set(0,0,0); 
	range = 0; 
	flags = collide::rq_target::rqtNone; 
	ignore = nullptr;
};

CRayPick::CRayPick(const Fvector& P, const Fvector& D, float R, collide::rq_target F, CScriptGameObject* I)
{
	start_position.set(P);
	direction.set(D);
	range = R;
	flags = F;
	ignore = nullptr;

	if (I)
	{
		ignore = smart_cast<CObject*>(&(I->object()));
	}
}

bool CRayPick::query()
{
    // COP_ENGINE_PATCH: COP-ENG-HF-002 - no stale hit or static lookup for dynamic bones.
    result.reset();
    material = std::numeric_limits<u16>::max();
	collide::rq_result R;
	if (Level().ObjectSpace.RayPick(start_position, direction, range, flags, R, ignore))
	{
		result.set(R);
        if (!R.O && R.element >= 0 && R.element < Level().ObjectSpace.GetStaticModel()->get_tris_count())
            material = (u16)Level().ObjectSpace.GetStaticTris()[R.element].material;
		return true;
	}

	return false;
}