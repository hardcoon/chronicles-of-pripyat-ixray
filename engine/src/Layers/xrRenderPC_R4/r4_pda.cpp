#include "stdafx.h"
#include "r4_rendertarget.h"

// COP_ENGINE_PATCH: COP-ENG-212 - render-to-texture bridge without changing shared vtables.
Fvector4 pf_pda_params = {0.f, 1.f, 0.f, 0.f};
namespace
{
    ID3DRenderTargetView* savedRT[4] = {};
    ID3DDepthStencilView* savedDepth = nullptr;
    D3D_VIEWPORT savedViewport[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT savedViewportCount = 0;
    u32 savedWidth = 0, savedHeight = 0;
    bool drawing = false;
}

// COP_ENGINE_PATCH: COP-ENG-221 - keep the projection of these HUD models in camera space.
// The ordinary world transform is still used for lights and all other world-space effects.
namespace
{
    struct PdaHudPose
    {
        u32 frame = u32(-1);
        IKinematics* hands = nullptr;
        IKinematics* item = nullptr;
        IKinematics* watch = nullptr;
        Fmatrix handView, itemView;

        const Fmatrix* find(u32 drawFrame, IKinematics* model) const
        {
            if (frame != drawFrame) return nullptr;
            if (model == item) return &itemView;
            return model == hands || model == watch ? &handView : nullptr;
        }
    };
    PdaHudPose readPoses[2];

    void setViewPose(const Fmatrix& pose, const Fmatrix* bone, bool previous)
    {
        auto& transforms = RCache.xforms;
        Fmatrix& view = previous ? transforms.m_wv_old : transforms.m_wv;
        Fmatrix& clip = previous ? transforms.m_wvp_old : transforms.m_wvp;
        if (bone) view.mul_43(pose, *bone);
        else view.set(pose);
        clip.mul(previous ? transforms.m_p_old : transforms.m_p, view);
        if (auto* c = previous ? transforms.c_wv_old : transforms.c_wv) RCache.set_c(c, view);
        if (auto* c = previous ? transforms.c_wvp_old : transforms.c_wvp) RCache.set_c(c, clip);
    }
}

extern "C" __declspec(dllexport) void __cdecl xrSetPdaHudPose(IKinematics* hands, IKinematics* item, IKinematics* watch,
    const Fmatrix* handView, const Fmatrix* itemView)
{
    auto& pose = readPoses[Device.dwFrame & 1];
    pose.frame = Device.dwFrame;
    pose.hands = hands;
    pose.item = item;
    pose.watch = watch;
    pose.handView = *handView;
    pose.itemView = *itemView;
}

void ApplyPdaHudPose(IKinematics* model, const Fmatrix* bone, const Fmatrix* oldBone)
{
    if (!Device.hudViewportData.isRenderProcess) return;
    const u32 frame = Device.dwFrame;
    const auto* pose = readPoses[frame & 1].find(frame, model);
    if (!pose) return;
    setViewPose(*pose, bone, false);
    if (const auto* old = readPoses[(frame - 1) & 1].find(frame - 1, model))
        setViewPose(*old, oldBone, true);
}

// COP_ENGINE_PATCH: COP-ENG-219 - power also changes while a closed dialog retains its last frame.
extern "C" __declspec(dllexport) void __cdecl xrSetPdaBrightness(float brightness)
{
    pf_pda_params.x = _valid(brightness) ? clampr(brightness, 0.f, 1.f) : 0.f;
}

extern "C" __declspec(dllexport) BOOL __cdecl xrPdaBegin(float alpha)
{
    auto* target = RImplementation.Target;
    if (!target || drawing || !RContext) return FALSE;
    xrSetPdaBrightness(alpha);
    auto& texture = target->rt_pf_pda;
    if (texture && (texture->dwWidth != Device.TargetWidth || texture->dwHeight != Device.TargetHeight)) texture.destroy();
    // COP_ENGINE_PATCH: COP-ENG-216 - regenerate filtered levels for the live UI every frame.
    // CRT's generic reset recreates a single level; repair only this opt-in texture after reset.
    if (texture && texture->pSurface) {
        D3D11_TEXTURE2D_DESC desc = {};
        texture->pSurface->GetDesc(&desc);
        if (!(desc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS)) texture.destroy();
    }
    if (!texture) texture.create("$user$pf_pda", Device.TargetWidth, Device.TargetHeight,
        DxgiFormat::DXGI_FORMAT_R8G8B8A8_UNORM, 1, CRT::AUTOGEN_MIPS_RT_FLAG);
    if (!texture) return FALSE;
    savedWidth = target->get_width(); savedHeight = target->get_height();
    RContext->OMGetRenderTargets(4, savedRT, &savedDepth);
    savedViewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    RContext->RSGetViewports(&savedViewportCount, savedViewport);
    drawing = true;
    target->u_setrt(texture, nullptr, nullptr, nullptr);
    RCache.set_RT(nullptr, 3);
    RImplementation.rmNormal();
    RCache.set_Stencil(FALSE); RCache.set_Z(FALSE); RCache.set_Scissor(nullptr); RCache.set_ColorWriteEnable();
    const FLOAT clear[4] = {0.f, 0.f, 0.f, 1.f};
    RContext->ClearRenderTargetView(texture->pRT, clear);
    return TRUE;
}

extern "C" __declspec(dllexport) void __cdecl xrPdaEnd()
{
    if (!drawing) return;
    RImplementation.Target->u_setrt(savedWidth, savedHeight, savedRT[0], savedRT[1], savedRT[2], savedDepth);
    RCache.set_RT(savedRT[3], 3);
    // Unbind before GenerateMips; do not sample a render target still bound for writing.
    RContext->OMSetRenderTargets(4, savedRT, savedDepth);
    auto& texture = RImplementation.Target->rt_pf_pda;
    if (texture && texture->pTexture) RContext->GenerateMips(texture->pTexture->get_SRView());
    RContext->RSSetViewports(savedViewportCount, savedViewport);
    for (auto*& view : savedRT) _RELEASE(view);
    _RELEASE(savedDepth);
    drawing = false;
}
