#include "stdafx.h"

#include "OverlayAPI\DLSSWrapper.h"
#include "r4_dlss_context.h"
#include "../xrRender/SSSSettings.h"
#include "../../xrEngine/IGame_Level.h"

extern Fvector3 ps_r_taa_jitter_full;

void CRenderTarget::init_dlss(bool createNow)
{
    // COP_ENGINE_PATCH: COP-ENG-163 - SDK creation can change immediate-context state.
    DLSSContextScope context(RContext, [] { RCache.OnFrameEnd(); });
    reset_dlss_history();
	g_DLSSWrapper.Destroy();
    // COP_ENGINE_PATCH: COP-ENG-177 - constructor/reset only retires the old feature.
    // Create it after scene uploads, precache and a complete valid render frame.
    if (!createNow) return;


	DLSSWrapper::ContextParameters initParams;
	initParams.device = RDevice;
	initParams.displaySize = { (int)RCache.get_target_width(), (int)RCache.get_target_height() };
	initParams.renderSize = { (int)RCache.get_width(), (int)RCache.get_height() };
	g_DLSSWrapper.Create(initParams);
}

bool CRenderTarget::phase_dlss()
{
	GPU_EVENT(DLSS);
    // COP_ENGINE_PATCH: COP-ENG-163 - detach SSS inputs and invalidate SDK side effects.
    DLSSContextScope context(RContext, [] { RCache.OnFrameEnd(); });
    DLSSFrameState frame;
    frame.frame = Device.dwFrame;
    frame.revision = ps_ssfx_revision;
    frame.width = rt_Generic_0->dwWidth;
    frame.height = rt_Generic_0->dwHeight;
    frame.secondaryViewport = Device.hudViewportData.isRenderActive;
    frame.position[0] = Device.vCameraPosition.x; frame.position[1] = Device.vCameraPosition.y; frame.position[2] = Device.vCameraPosition.z;
    frame.direction[0] = Device.vCameraDirection.x; frame.direction[1] = Device.vCameraDirection.y; frame.direction[2] = Device.vCameraDirection.z;
    frame.fov = Device.fFOV;

    // COP_ENGINE_PATCH: COP-ENG-177 - do not feed loading/precache or invalid-camera frames to NGX.
    const bool sceneReady = g_pGameLevel && g_pGameLevel->bReady;
    const bool validCamera = dlss_activation::valid_camera(frame);
    const bool ready = dlss_scene_gate.update(frame.frame,sceneReady && !Device.dwPrecacheFrame && validCamera);
    const bool created = g_DLSSWrapper.IsCreated();
    const auto action = dlss_activation::action(ready,created,dlss_history.configurationChanged(frame));
    if (action == dlss_activation::Action::Defer) {
        if (created) init_dlss(false);
        else reset_dlss_history();
        phase_scale(1); // COP_ENGINE_PATCH: COP-ENG-178 - explicit linear loading pass; retain DLSS selection.
        return true;
    }
    if (action == dlss_activation::Action::Create || action == dlss_activation::Action::Recreate) {
        Msg("* [DLSS lifecycle177] %s frame=%u ready=%u precache=%u revision=%u render=%ux%u",
            action==dlss_activation::Action::Create ? "create_ready_scene":"recreate_configuration",
            frame.frame,unsigned(sceneReady),Device.dwPrecacheFrame,frame.revision,frame.width,frame.height);
        init_dlss(true);
    }
	DLSSWrapper::DrawParameters dlssParams = {};
	dlssParams.deviceContext = RContext;

	dlssParams.unresolvedColorResource = rt_Generic_0->pSurface;
	dlssParams.motionvectorResource = rt_Velocity->pSurface;
	dlssParams.depthbufferResource = rt_Position->pSurface;

	dlssParams.exposureResource = nullptr;
	dlssParams.reactiveMapResource = nullptr;
	dlssParams.transparencyAndCompositionResource = nullptr;

	dlssParams.resolvedColorResource = rt_Generic->pSurface;

	dlssParams.renderWidth = (int)RCache.get_width();
	dlssParams.renderHeight = (int)RCache.get_height();

    // COP_ENGINE_PATCH: COP-ENG-188 - bounded reasons distinguish zoom from scene discontinuities.
    const char* historyReset = dlss_history.resetReason(frame);
    dlssParams.cameraReset = historyReset != nullptr;
    static unsigned loggedHistoryResets = 0;
    if (historyReset && loggedHistoryResets < 32)
    {
        ++loggedHistoryResets;
        Msg("* [DLSS history188] reset reason=%s frame=%u fov=%.6g secondary=%u",
            historyReset, frame.frame, frame.fov, unsigned(frame.secondaryViewport));
    }

	dlssParams.cameraJitterX = ps_r_taa_jitter_full.x;
	dlssParams.cameraJitterY = ps_r_taa_jitter_full.y;

	dlssParams.sharpness = 0.0f;

	dlssParams.frameTimeDelta = std::max(1.0f + EPS_L, float(Device.dwTimeDelta));

	dlssParams.nearPlane = Device.fViewportNear;
	dlssParams.farPlane = g_pGamePersistent->Environment().CurrentEnv->far_plane;
	dlssParams.fovH = deg2rad(Device.fFOV);

	const bool success = g_DLSSWrapper.Draw(dlssParams);
    dlss_history.commit(frame, success);
    return success;
}
