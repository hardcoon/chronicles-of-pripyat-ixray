#include "stdafx.h"
#include "dxScriptDebugRender.h"

#ifdef USE_DX11
// COP_ENGINE_PATCH: COP-ENG-HF-001 - Release world-space script primitives.
dxScriptDebugRender ScriptDebugRenderImpl;

void dxScriptDebugRender::Create()
{
    shader.create("script_debug_lines");
    geometry.create(FVF::F_L, RCache.Vertex.Buffer(), nullptr);
    modelGeometry.create(FVF::F_LIT, RCache.Vertex.Buffer(), nullptr);
}

void dxScriptDebugRender::Destroy()
{
    ClearModels();
    modelGeometry.destroy();
    geometry.destroy();
    shader.destroy();
}

void dxScriptDebugRender::ClearModels()
{
    modelShaders.clear();
}

void dxScriptDebugRender::DrawTriangles(const ScriptPreviewVertex* vertices, u32 vertexCount, LPCSTR texture)
{
    if (!vertices || vertexCount < 3 || !modelGeometry || !texture || !*texture) return;
    auto found = modelShaders.find(shared_str(texture));
    if (found == modelShaders.end())
    {
        ref_shader value;
        value.create("script_preview_model", texture);
        found = modelShaders.emplace(shared_str(texture), value).first;
    }
    if (!found->second) return;
    const Fmatrix world = RCache.get_xform_world();
    const Fmatrix view = RCache.get_xform_view();
    const Fmatrix projection = RCache.get_xform_project();
    RCache.set_xform_world(Fidentity);
    RCache.set_xform_view(Device.mView);
    RCache.set_xform_project(Device.mProject);
    RCache.set_Element(found->second->E[0]);
    RCache.set_Geometry(modelGeometry);
    const u32 cullMode = RCache.get_CullMode();
    RCache.set_CullMode(CULL_NONE);
    vertexCount -= vertexCount % 3;
    for (u32 start = 0; start < vertexCount;)
    {
        const u32 count = std::min(4095u, vertexCount - start);
        u32 base = 0;
        auto* output = static_cast<FVF::LIT*>(RCache.Vertex.Lock(count, modelGeometry->vb_stride, base));
        for (u32 i = 0; i < count; ++i)
        {
            output[i].p = vertices[start + i].position;
            output[i].color = vertices[start + i].color;
            output[i].t = vertices[start + i].uv;
        }
        RCache.Vertex.Unlock(count, modelGeometry->vb_stride);
        RCache.Render(D3DPT_TRIANGLELIST, base, count / 3);
        start += count;
    }
    RCache.set_xform_world(world);
    RCache.set_xform_view(view);
    RCache.set_xform_project(projection);
    RCache.set_CullMode(cullMode);
}

void dxScriptDebugRender::DrawLines(const ScriptDebugVertex* vertices, u32 vertexCount)
{
    if (!vertices || vertexCount < 2 || !shader || !geometry)
        return;
    vertexCount -= vertexCount % 2;
    const Fmatrix world = RCache.get_xform_world();
    const Fmatrix view = RCache.get_xform_view();
    const Fmatrix projection = RCache.get_xform_project();
    RCache.set_xform_world(Fidentity);
    RCache.set_xform_view(Device.mView);
    RCache.set_xform_project(Device.mProject);
    RCache.set_Element(shader->E[0]);
    RCache.set_Geometry(geometry);

    // Counts are vertices, never pairs of vertices. Keep each batch even.
    const u32 batchLimit = 4096;
    for (u32 start = 0; start < vertexCount;)
    {
        const u32 count = std::min(batchLimit, vertexCount - start);
        u32 base = 0;
        FVF::L* output = static_cast<FVF::L*>(RCache.Vertex.Lock(count, geometry->vb_stride, base));
        for (u32 i = 0; i < count; ++i)
        {
            output[i].p = vertices[start + i].position;
            output[i].color = vertices[start + i].color;
        }
        RCache.Vertex.Unlock(count, geometry->vb_stride);
        RCache.Render(D3DPT_LINELIST, base, count / 2);
        start += count;
    }
    RCache.set_xform_world(world);
    RCache.set_xform_view(view);
    RCache.set_xform_project(projection);
}
#endif
