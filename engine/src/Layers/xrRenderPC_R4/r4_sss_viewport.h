#pragma once
// COP_ENGINE_PATCH: COP-ENG-136 - private SSS passes must not leak scaled viewports into native rendering.
class SSSViewportScope
{
    u32& width;
    u32& height;
    const u32 previousWidth;
    const u32 previousHeight;
    const bool active;
    u32 count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
public:
    SSSViewportScope(u32& w, u32& h, bool enabled = true)
        : width(w), height(h), previousWidth(w), previousHeight(h), active(enabled)
    {
        if (active) RContext->RSGetViewports(&count, viewports);
    }
    ~SSSViewportScope()
    {
        if (!active) return;
        width = previousWidth;
        height = previousHeight;
        RContext->RSSetViewports(count, viewports);
    }
    SSSViewportScope(const SSSViewportScope&) = delete;
    SSSViewportScope& operator=(const SSSViewportScope&) = delete;
};
