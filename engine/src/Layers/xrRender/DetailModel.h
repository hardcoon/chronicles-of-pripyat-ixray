#pragma once

#include "IRenderDetailModel.h"
#include "../../xrEngine/VehicleGrass.h"

class ECORE_API CDetail:
	public IRender_DetailModel
{
public:
	struct SlotItem
	{								// один кустик
		Fvector						hpb;
		float						scale_calculated;
		Fvector						pos;
		float						c_hemi;


        // COP_ENGINE_PATCH: COP-ENG-170 - transient cache state and previous-frame deformation.
        struct GrassFrame { VehicleGrass::Deformation current, previous; };
        GrassFrame grass[2]; // Worker writes calc_key; renderer reads the other render_key.
        u32 grass_frame = u32(-1);
        u8 grass_last_key = 0;
        bool grass_keep = false;
        VehicleGrass::Deformation grass_target; // COP_ENGINE_PATCH: COP-ENG-171 - worker-only contact state.
		float						scale;
		u8							vis_ID;				// индекс в visibility списке он же тип [не качается, качается1, качается2]
	};
	
#ifdef USE_DX11
	ref_geom					hw_Geom;
	ID3DVertexBuffer*			hw_VB;
	ID3DIndexBuffer*			hw_IB;	
#endif
	
	xr_vector<xr_shared_ptr<SlotItem>> m_items[3][2];
	void			Load		(IReader* S);
	void			Optimize	();
	virtual void	Unload		();

	virtual void	transfer	(Fmatrix& mXform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset);
	virtual void	transfer	(Fmatrix& mXform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset, float du, float dv);
	virtual			~CDetail	();
};
