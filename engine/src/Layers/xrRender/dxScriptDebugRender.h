#pragma once
#ifdef USE_DX11
#include "../../Include/xrRender/ScriptDebugRender.h"

class dxScriptDebugRender final : public IScriptDebugRender
{
    ref_shader shader;
    ref_geom geometry;
    ref_geom modelGeometry;
    xr_map<shared_str, ref_shader> modelShaders;
public:
    void Create();
    void Destroy();
    void DrawLines(const ScriptDebugVertex* vertices, u32 vertexCount) override;
    void DrawTriangles(const ScriptPreviewVertex* vertices, u32 vertexCount, LPCSTR texture) override;
    void ClearModels() override;
};
extern dxScriptDebugRender ScriptDebugRenderImpl;
#endif
