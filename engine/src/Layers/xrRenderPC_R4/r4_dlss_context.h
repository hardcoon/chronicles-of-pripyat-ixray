#pragma once
#include <d3d11.h>

// COP_ENGINE_PATCH: COP-ENG-163 - NGX uses the immediate context outside RCache.
// Clear both sides of the boundary: no SSS SRV/RTV/UAV hazards on entry, and
// no SDK bindings left behind while RCache still describes the previous pass.
class DLSSContextScope final
{
    ID3D11DeviceContext* context;
    void (*invalidate)();

    void reset()
    {
        context->ClearState();
        invalidate();
    }
public:
    DLSSContextScope(ID3D11DeviceContext* ctx, void (*resetCache)())
        : context(ctx), invalidate(resetCache) { reset(); }
    ~DLSSContextScope() { reset(); }
    DLSSContextScope(const DLSSContextScope&) = delete;
    DLSSContextScope& operator=(const DLSSContextScope&) = delete;
};
