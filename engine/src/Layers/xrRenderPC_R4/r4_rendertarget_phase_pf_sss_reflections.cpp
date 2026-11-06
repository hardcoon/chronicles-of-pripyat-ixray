#include "stdafx.h"
#include "r4_sss_viewport.h"
#include "r4_rendertarget.h"
#include "blender_pf_sss_reflections.h"
#include "../xrRender/SSSSettings.h"

// COP_ENGINE_PATCH: COP-ENG-140 - the existing active preparation advances the shared clock once per frame.
extern float ssfx_update_puddle_wetness();
extern float ps_pf_sss_puddle_wetness;

// COP_ENGINE_PATCH: COP-ENG-128 - fifth MRT never replaces native velocity/HUD mask.
void CRenderTarget::prepare_pf_sss_data() {
    string_path path;
    // COP_ENGINE_PATCH: COP-ENG-138 - allocate puddle metadata only while the effect is enabled.
    const bool active = ssfx_puddles_active() && FS.exist(path, "$game_shaders$", "d3d11\\pf_sss_ssr.ps.hlsl");
    if (!active) {
        ps_pf_sss_puddle_wetness=-1.f;
        if (rt_pf_sss_data) rt_pf_sss_data.destroy();
        reset_pf_sss_water(); rt_pf_sss_scene.destroy(); rt_pf_sss_velocity.destroy(); reset_pf_sss_sun(); reset_pf_sss_postfx();reset_pf_sss_volume();
        reset_pf_sss_reflections();
        return;
    }
    ssfx_update_puddle_wetness();
    if (!rt_pf_sss_data) {
        rt_pf_sss_data.create("$user$pf_sss_data", rt_Color->dwWidth, rt_Color->dwHeight,
            DxgiFormat::DXGI_FORMAT_R8G8B8A8_UNORM);
        Msg("[SSS] material target active; native settings preserved (eng-128)");
    }
    const FLOAT clear[4] = {};
    RContext->ClearRenderTargetView(rt_pf_sss_data->pRT, clear);
}

void CRenderTarget::snapshot_pf_sss_scene() {
    if (!ssfx_active() || !rt_pf_sss_scene) return;
    u_setrt(rt_Generic_0->dwWidth, rt_Generic_0->dwHeight, nullptr, nullptr, nullptr, nullptr);
    RContext->CopyResource(rt_pf_sss_scene->pSurface, rt_Generic_0->pSurface);
    prepare_pf_sss_water();
}

void CRenderTarget::reset_pf_sss_reflections() {
    s_pf_sss_reflections.destroy();
    rt_pf_sss_ssr.destroy(); rt_pf_sss_ssr_previous.destroy(); rt_pf_sss_ssr_filter.destroy();
    rt_pf_sss_il.destroy(); rt_pf_sss_il_previous.destroy(); rt_pf_sss_il_filter.destroy();
    rt_pf_sss_composite.destroy();
    pf_sss_reflection_history = false;
}

void CRenderTarget::phase_pf_sss_reflections() {
    if (!ssfx_puddles_active() || !rt_pf_sss_data) { reset_pf_sss_reflections(); return; }
    const bool ssr = ssfx_option(SSS_ssr_enabled) > .5f;
    if (!ssr) { reset_pf_sss_reflections(); return; }
    GPU_EVENT(phase_pf_sss_reflections);
    const u32 width = rt_Generic_0->dwWidth, height = rt_Generic_0->dwHeight;
    const u32 sw = _max(1u, u32(width * ssfx_option(SSS_ssr_render_scale)));
    const u32 sh = _max(1u, u32(height * ssfx_option(SSS_ssr_render_scale)));
    if (s_pf_sss_reflections && (rt_pf_sss_ssr->dwWidth != sw || rt_pf_sss_ssr->dwHeight != sh)) reset_pf_sss_reflections();
    if (!s_pf_sss_reflections) {
        const auto format = DxgiFormat::DXGI_FORMAT_R16G16B16A16_FLOAT;
        rt_pf_sss_ssr.create("$user$pf_sss_ssr", sw, sh, format);
        rt_pf_sss_ssr_previous.create("$user$pf_sss_ssr_previous", sw, sh, format);
        rt_pf_sss_ssr_filter.create("$user$pf_sss_ssr_filter", sw, sh, format);
        rt_pf_sss_composite.create("$user$pf_sss_composite", width, height, format);
        CBlender_pf_sss_reflections blender;
        s_pf_sss_reflections.create(&blender);
        Msg("[SSS] puddle reflections %ux%u; no IL buffers", sw, sh);
    }
    const bool history = pf_sss_reflection_history && pf_sss_reflection_frame + 1 == Device.dwFrame
        && pf_sss_reflection_revision == ps_ssfx_revision
        && Device.vCameraPosition.distance_to_sqr(pf_sss_reflection_camera) < 4.f
        && Device.vCameraDirection.dotproduct(pf_sss_reflection_direction)>.8f
        && _abs(Device.fFOV-pf_sss_reflection_fov)<.01f && pf_sss_reflection_svp==Device.hudViewportData.isRenderActive;
    if (!history) {
        const FLOAT clear[4] = {};
        RContext->ClearRenderTargetView(rt_pf_sss_ssr_previous->pRT, clear);
    }
    auto draw = [&](const ref_rt& target, u32 element) {
        SSSViewportScope viewport(dwWidth, dwHeight);
        u_setrt(target, nullptr, nullptr, nullptr);
        RImplementation.rmNormal();
        RCache.set_Scissor(nullptr);
        RCache.set_CullMode(CULL_NONE); RCache.set_Stencil(FALSE); RCache.set_ColorWriteEnable();
        u32 offset = 0;
        auto* vertices = (FVF::TL*)RCache.Vertex.Lock(3, g_combine->vb_stride, offset);
        vertices[0].set(-1.f, 1.f, 1.f, 1.f, 0, 0.f, 0.f);
        vertices[1].set(3.f, 1.f, 1.f, 1.f, 0, 2.f, 0.f);
        vertices[2].set(-1.f, -3.f, 1.f, 1.f, 0, 0.f, 2.f);
        RCache.Vertex.Unlock(3, g_combine->vb_stride);
        RCache.set_Element(s_pf_sss_reflections->E[element]);
        RCache.set_c("pf_sss_history", history ? 1.f : 0.f, 0.f, 0.f, 0.f);
        RCache.set_Geometry(g_combine);
        RCache.Render(D3DPT_TRIANGLELIST, offset, 0, 3, 0, 1);
    };
    auto composite = [&](u32 element) {
        draw(rt_pf_sss_composite, element);
        // Unbind RTV/SRVs before copying; the next pass reads the new HDR image.
        u_setrt(width, height, nullptr, nullptr, nullptr, nullptr);
        RImplementation.rmNormal();
        RContext->CopyResource(rt_Generic_0->pSurface, rt_pf_sss_composite->pSurface);
    };
    if (ssr) {
        draw(rt_pf_sss_ssr, 0); draw(rt_pf_sss_ssr_filter, 1);
        composite(2);
        RContext->CopyResource(rt_pf_sss_ssr_previous->pSurface, rt_pf_sss_ssr->pSurface);
    }
    pf_sss_reflection_history = true;
    pf_sss_reflection_frame = Device.dwFrame;
    pf_sss_reflection_revision = ps_ssfx_revision;
    pf_sss_reflection_camera = Device.vCameraPosition;
    pf_sss_reflection_direction=Device.vCameraDirection;pf_sss_reflection_fov=Device.fFOV;pf_sss_reflection_svp=Device.hudViewportData.isRenderActive;
}

void CRenderTarget::reset_pf_sss_water() {
    s_pf_sss_water_down.destroy();s_pf_sss_water_filter.destroy();s_pf_sss_water_tmp.destroy();
    rt_pf_sss_water_color.destroy();rt_pf_sss_water_filter.destroy();rt_pf_sss_water_tmp.destroy();
}
void CRenderTarget::prepare_pf_sss_water() {
    // COP_ENGINE_PATCH: COP-ENG-128 - real scaled HDR targets; separate water blur avoids forward RTV feedback.
    if(!ssfx_active() || ssfx_option(SSS_water_enabled)<.5f){reset_pf_sss_water();return;}
    const u32 w=_max(1u,u32(rt_pf_sss_scene->dwWidth*ssfx_option(SSS_water_render_scale)));
    const u32 h=_max(1u,u32(rt_pf_sss_scene->dwHeight*ssfx_option(SSS_water_render_scale)));
    if(rt_pf_sss_water_color && (rt_pf_sss_water_color->dwWidth!=w || rt_pf_sss_water_color->dwHeight!=h))reset_pf_sss_water();
    if(!rt_pf_sss_water_color){
        const auto format=DxgiFormat::DXGI_FORMAT_R16G16B16A16_FLOAT;
        rt_pf_sss_water_color.create("$user$pf_sss_water_color",w,h,format);
        rt_pf_sss_water_tmp.create("$user$pf_sss_water_tmp",w,h,format);
        rt_pf_sss_water_filter.create("$user$pf_sss_water_filter",w,h,format);
        CBlender_pf_sss_water down("$user$pf_sss_scene"),tmp("$user$pf_sss_water_color"),filter("$user$pf_sss_water_tmp");
        s_pf_sss_water_down.create(&down);s_pf_sss_water_tmp.create(&tmp);s_pf_sss_water_filter.create(&filter);
    }
    const float radius=ssfx_option(SSS_water_blur)*6.f;
    draw_pf_sss_postfx(rt_pf_sss_water_color,s_pf_sss_water_down,0,Fvector4().set(float(w),float(h),0,0));
    draw_pf_sss_postfx(rt_pf_sss_water_tmp,s_pf_sss_water_tmp,1,Fvector4().set(float(w),float(h),radius,0));
    draw_pf_sss_postfx(rt_pf_sss_water_filter,s_pf_sss_water_filter,1,Fvector4().set(float(w),float(h),0,radius));
}
