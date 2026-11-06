#include "stdafx.h"
#include "r4_sss_viewport.h"
#include "r4_rendertarget.h"
#include "blender_pf_sss.h"
#include "../xrRender/SSSSettings.h"

// COP_ENGINE_PATCH: COP-ENG-126 - off restores native paths without changing their settings.
void CRenderTarget::reset_pf_sss()
{
    s_pf_sss.destroy();
    rt_pf_sss_current.destroy();
    rt_pf_sss_previous.destroy();
    pf_sss_history_valid = false;
}

// COP_ENGINE_PATCH: COP-ENG-138 - native AO remains in charge for every retained effect.
bool CRenderTarget::phase_pf_sss_ao()
{
    if (s_pf_sss) reset_pf_sss();
    return false;
}
