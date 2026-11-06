#include "stdafx.h"

#include "../../xrEngine/IGame_Persistent.h"
#include "../../xrEngine/Environment.h"

#pragma pack(push,4)
struct v_build	{
	Fvector4	p;
	Fvector2	uv0;
	Fvector2	uv1;
	Fvector2	uv2;
	Fvector2	uv3;
};

#pragma pack(pop)

void CRenderTarget::phase_bloom	()
{
	GPU_EVENT(phase_bloom);
	u32		Offset;

	// Targets
	u_setrt									( rt_Bloom_1,nullptr,nullptr,nullptr );		// No need for ZBuffer at all
	//RImplementation.rmNormal();
	
	// Clear	- don't clear - it's stupid here :)
	// Stencil	- disable
	// Misc		- draw everything (no culling)
	//CHK_DX		(RDevice->SetRenderState	( D3DRS_ZENABLE,		FALSE				));
	RCache.set_Z(FALSE);

	// Transfer into Bloom1
	{
		float		_w				= RCache.get_width();
		float		_h				= RCache.get_height();
		float		_2w				= _w/2;	float tw = BLOOM_size_X;
		float		_2h				= _h/2; float th = BLOOM_size_Y;
		float		_aspect_w		= _2w/tw;
		float		_aspect_h		= _2h/th;
		Fvector2	one				= { 1.f/_w, 1.f/_h };	one.x*=_aspect_w; one.y*=_aspect_h;
		Fvector2	half			= { .5f/_w, .5f/_h };
		Fvector2	a_0				= { half.x + 0,		half.y + 0		};
		Fvector2	a_1				= { half.x + one.x, half.y + 0		};
		Fvector2	a_2				= { half.x + 0,		half.y + one.y	};
		Fvector2	a_3				= { half.x + one.x,	half.y + one.y	};
		Fvector2	b_0				= { 1 + a_0.x,		1 + a_0.y		};
		Fvector2	b_1				= { 1 + a_1.x,		1 + a_1.y		};
		Fvector2	b_2				= { 1 + a_2.x,		1 + a_2.y		};
		Fvector2	b_3				= { 1 + a_3.x,		1 + a_3.y		};

		// Fill vertex buffer
		v_build* pv					= (v_build*) RCache.Vertex.Lock	(4,g_bloom_build->vb_stride,Offset);
		pv->p.set	(EPS,			float(th+EPS),	EPS,1.f);	
		pv->uv0.set	(a_0.x,b_0.y);	pv->uv1.set	(a_1.x,b_1.y);	pv->uv2.set	(a_2.x,b_2.y);	pv->uv3.set	(a_3.x,b_3.y);
		pv++;
		pv->p.set	(EPS,			EPS,			EPS,1.f);	
		pv->uv0.set	(a_0.x,a_0.y);	pv->uv1.set	(a_1.x,a_1.y);	pv->uv2.set	(a_2.x,a_2.y);	pv->uv3.set	(a_3.x,a_3.y);
		pv++;
		pv->p.set	(float(tw+EPS),float(th+EPS),	EPS,1.f);	
		pv->uv0.set	(b_0.x,b_0.y);	pv->uv1.set	(b_1.x,b_1.y);	pv->uv2.set	(b_2.x,b_2.y);	pv->uv3.set	(b_3.x,b_3.y);
		pv++;
		pv->p.set	(float(tw+EPS),EPS,			EPS,1.f);	
		pv->uv0.set	(b_0.x,a_0.y);	pv->uv1.set	(b_1.x,a_1.y);	pv->uv2.set	(b_2.x,a_2.y);	pv->uv3.set	(b_3.x,a_3.y);
		pv++;
		RCache.Vertex.Unlock		(4,g_bloom_build->vb_stride);

		// Perform combine (all scalers must account for 4 samples + final diffuse multiply);
		float s						= ps_r2_ls_bloom_threshold;											// scale
		f_bloom_factor				= .9f*f_bloom_factor + .1f*ps_r2_ls_bloom_speed*Device.fTimeDelta;	// speed
		RCache.set_Element(s_bloom->E[0]);
		RCache.set_c				("b_params", s,s,s,	f_bloom_factor);
		RCache.set_Geometry		(g_bloom_build		);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}

	// Capture luminance values
	phase_luminance					( );
    // COP_ENGINE_PATCH: COP-ENG-128 - native luminance adaptation stays above.
    // COP_ENGINE_PATCH: COP-ENG-218 - combine shares this exposure source.
    // After metering, provide a neutral contribution to every combine shader.
    const FLOAT black[4] = {};
    RContext->ClearRenderTargetView(rt_Bloom_1->pRT, black);
}
