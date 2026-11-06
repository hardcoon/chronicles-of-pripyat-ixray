#include "stdafx.h"

void CRenderTarget::phase_scale(u32 filter) {
	u32 Offset = 0;
	Fvector2 p0, p1;

	u_setrt(rt_Generic, 0, 0, 0);
	RImplementation.rmNormal();

	constexpr auto C = color_rgba(255, 255, 255, 255);
	float _w = RCache.get_target_width();
	float _h = RCache.get_target_height();
	float d_Z = EPS_S;
	float d_W = 1.f;

	p0.set(.5f / _w, .5f / _h);
	p1.set((_w + .5f) / _w, (_h + .5f) / _h);

	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);

	pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);	pv++;
	pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);	pv++;
	pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);	pv++;
	pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);	pv++;

	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	// Draw COLOR
	// COP_ENGINE_PATCH: COP-ENG-178 - only spatial elements0/1 exist; DLSS is mode2.
    const u32 element = filter == u32(-1) ? ps_r_scale_mode : filter;
    RCache.set_Element(s_scale->E[element <= 1 ? element : 1]);
	RCache.set_Geometry(g_combine);

	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}