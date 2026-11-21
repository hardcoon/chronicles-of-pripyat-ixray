#include "stdafx.h"
#include "PFSkybox.h"
#include "r4_rendertarget.h"
#include "../../xrEngine/IGame_Persistent.h"
#include "../../xrEngine/Environment.h"
#include "../xrRenderDX10/StateManager/dx10ShaderResourceStateCache.h"
#include <cmath>
#include <string>

// COP_ENGINE_PATCH: COP-ENG-244 - independent miniature scene, no ALife/save data.
// COP_ENGINE_PATCH: COP-ENG-245 - late composite with native depth/fog rejection.
// COP_ENGINE_PATCH: COP-ENG-246 - textured PFSB v2 and instanced 3-D tree LODs.
namespace
{
struct SkyVertex { Fvector position, normal; u32 color; };
struct MaterialVertex { Fvector position, normal; Fvector2 uv; u32 color; };
struct Instance { Fvector4 positionHeight, rotationWidthType; };
struct FilePart
{
    u32 firstIndex, indexCount, baseVertex, vertexCount;
    u32 firstInstance, instanceCount, mode, reserved;
    char texture[128];
};
static_assert(sizeof(SkyVertex) == 28, "PFSB v1 layout changed");
static_assert(sizeof(MaterialVertex) == 36 && sizeof(Instance) == 32 && sizeof(FilePart) == 160, "PFSB v2 layout changed");
struct Group { u32 first, count, mode, visibleFirst = 0, visibleCount = 0; };
struct Part { FilePart data {}; u32 group = 0; ref_shader shader; };
struct Scene
{
    xr_vector<SkyVertex> vertices;
    xr_vector<MaterialVertex> materialVertices;
    xr_vector<u16> indices;
    xr_vector<Instance> instances, visibleInstances;
    xr_vector<Part> parts;
    xr_vector<Group> groups;
    ref_shader shader;
    ref_geom geometry;
    ref_rt depth;
    ID3DVertexBuffer* buffer = nullptr;
    ID3DIndexBuffer* indexBuffer = nullptr;
    ID3D11Buffer* instanceBuffer = nullptr;
    ID3D11ShaderResourceView* instanceView = nullptr;
    Fvector4 bounds {};
    Fvector previousCamera {};
    u32 version = 1, instanceCapacity = 0, width = 0, height = 0, lastFrame = 0;
    float scale = 16.f, hemi = .8f, exposure = 1.f;
    bool enabled = false, gpuReady = false, gpuFailed = false;
};
Scene scene;
bool finiteVector(const Fvector& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
bool finite4(const Fvector4& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.w); }
bool validVertex(const Fvector& p, const Fvector& n)
{
    const float norm = n.square_magnitude();
    return finiteVector(p) && finiteVector(n) && fabsf(p.x) <= 20000.f && fabsf(p.y) <= 20000.f && fabsf(p.z) <= 20000.f && std::isfinite(norm) && norm >= .9f && norm <= 1.1f;
}
float option(const CInifile& settings, LPCSTR section, LPCSTR key, float fallback)
{ return settings.line_exist(section, key) ? settings.r_float(section, key) : fallback; }
std::string levelName()
{
    const FS_Path* level = FS.get_path("$level$");
    if (!level || !level->m_Path) return {};
    std::string name(level->m_Path);
    while (!name.empty() && (name.back() == '\\' || name.back() == '/')) name.pop_back();
    const auto slash = name.find_last_of("\\/");
    if (slash != std::string::npos) name.erase(0, slash + 1);
    return name;
}
bool relativeAsset(LPCSTR name)
{ return name && *name && xr_strlen(name) < 240 && !strstr(name, "..") && !strchr(name, ':') && name[0] != '/' && name[0] != '\\'; }
bool textureExists(LPCSTR name)
{
    if (!relativeAsset(name)) return false;
    xr_string path(name); path += ".dds";
    return !!FS.exist("$game_textures$", path.c_str());
}
bool shaderExists(LPCSTR name)
{ xr_string path("d3d11\\"); path += name; path += ".xml"; return !!FS.exist("$game_shaders$", path.c_str()); }
bool loadLegacy(IReader& file, u32 count, u32 reserved)
{
    if (reserved || count < 3 || count % 3 || count > 750000 || u64(file.length()) != 16ull + u64(count) * sizeof(SkyVertex)) return false;
    if (!shaderExists("pf_skybox")) return false;
    scene.vertices.resize(count); file.r(scene.vertices.data(), count * sizeof(SkyVertex));
    for (const auto& v : scene.vertices) if (!validVertex(v.position, v.normal)) return false;
    return true;
}
bool loadMaterials(IReader& file, u32 vertexCount, u32 indexCount)
{
    // Exact byte bounds precede size-dependent allocation and reads.
    if (file.length() < 48 || file.length() > 64 * 1024 * 1024) return false;
    const u32 partCount = file.r_u32(), instanceCount = file.r_u32();
    const u32 reserved = file.r_u32(), flags = file.r_u32(); file.r(&scene.bounds, sizeof(scene.bounds));
    const u64 expected = 48ull + u64(vertexCount) * sizeof(MaterialVertex) + u64(indexCount) * sizeof(u16) + u64(partCount) * sizeof(FilePart) + u64(instanceCount) * sizeof(Instance);
    if (reserved || flags || vertexCount < 3 || vertexCount > 1000000 || indexCount < 3 || indexCount % 3 || indexCount > 3000000 || !partCount || partCount > 64 || !instanceCount || instanceCount > 100000 || expected != file.length()) return false;
    if (!finite4(scene.bounds) || scene.bounds.z - scene.bounds.x < 1.f || scene.bounds.w - scene.bounds.y < 1.f || scene.bounds.z - scene.bounds.x > 400000.f || scene.bounds.w - scene.bounds.y > 400000.f) return false;
    if (!shaderExists("pf_skybox_ground") || !shaderExists("pf_skybox_model") || !shaderExists("pf_skybox_billboard")) return false;
    for (LPCSTR texture : {"pf_skybox\\detail_grass", "pf_skybox\\detail_earth", "pf_skybox\\detail_asphalt", "pf_skybox\\detail_normal"}) if (!textureExists(texture)) return false;
    scene.materialVertices.resize(vertexCount); file.r(scene.materialVertices.data(), vertexCount * sizeof(MaterialVertex));
    scene.indices.resize(indexCount); file.r(scene.indices.data(), indexCount * sizeof(u16));
    for (const auto& v : scene.materialVertices)
        if (!validVertex(v.position, v.normal) || !std::isfinite(v.uv.x) || !std::isfinite(v.uv.y) || fabsf(v.uv.x) > 100000.f || fabsf(v.uv.y) > 100000.f) return false;
    scene.parts.resize(partCount);
    for (auto& part : scene.parts)
    {
        auto& p = part.data; file.r(&p, sizeof(p));
        if (p.reserved || p.mode > 2 || p.vertexCount < 3 || p.vertexCount > 65535 || p.indexCount < 3 || p.indexCount % 3 || u64(p.baseVertex) + p.vertexCount > vertexCount || u64(p.firstIndex) + p.indexCount > indexCount || !p.instanceCount || u64(p.firstInstance) + p.instanceCount > instanceCount || !memchr(p.texture, 0, sizeof(p.texture)) || !textureExists(p.texture)) return false;
        if (p.mode == 0 && (p.firstInstance != 0 || p.instanceCount != 1)) return false;
        for (u32 i = p.firstIndex; i < p.firstIndex + p.indexCount; ++i) if (scene.indices[i] >= p.vertexCount) return false;
        u32 group = 0;
        for (; group < scene.groups.size(); ++group)
        { const auto& g = scene.groups[group]; if (g.first == p.firstInstance && g.count == p.instanceCount && g.mode == p.mode) break; }
        if (group == scene.groups.size())
        {
            scene.groups.push_back({p.firstInstance, p.instanceCount, p.mode});
            if (u64(scene.instanceCapacity) + p.instanceCount > 100000) return false;
            scene.instanceCapacity += p.instanceCount;
        }
        part.group = group;
    }
    scene.instances.resize(instanceCount); file.r(scene.instances.data(), instanceCount * sizeof(Instance));
    for (const auto& i : scene.instances)
    {
        const auto& p = i.positionHeight; const auto& r = i.rotationWidthType;
        const float rotation = r.x * r.x + r.y * r.y;
        if (!finite4(p) || !finite4(r) || fabsf(p.x) > 20000.f || fabsf(p.y) > 20000.f || fabsf(p.z) > 20000.f || p.w <= 0.f || p.w > 500.f || rotation < .98f || rotation > 1.02f || r.z <= 0.f || r.z > 8.f || r.w < 0.f || r.w > 7.f) return false;
    }
    scene.visibleInstances.reserve(scene.instanceCapacity); return true;
}
bool createBuffer(UINT bytes, UINT bind, const void* data, ID3D11Buffer** buffer)
{
    D3D11_BUFFER_DESC description {}; description.ByteWidth = bytes; description.Usage = D3D11_USAGE_IMMUTABLE; description.BindFlags = bind;
    D3D11_SUBRESOURCE_DATA initial {}; initial.pSysMem = data;
    return SUCCEEDED(RDevice->CreateBuffer(&description, &initial, buffer));
}
bool createGPU()
{
    if (scene.gpuFailed) return false;
    if (scene.gpuReady) return true;
    bool ok = false;
    if (scene.version == 1)
    {
        ok = !scene.vertices.empty() && createBuffer(u32(scene.vertices.size() * sizeof(SkyVertex)), D3D11_BIND_VERTEX_BUFFER, scene.vertices.data(), &scene.buffer);
        if (ok)
        {
            D3DVERTEXELEMENT9 declaration[] = {{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0}, {0,12,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_NORMAL,0}, {0,24,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0}, D3DDECL_END()};
            scene.geometry.create(declaration, scene.buffer, nullptr); scene.shader.create("pf_skybox");
            Msg("* [PF SKYBOX] GPU ready triangles=%u", unsigned(scene.vertices.size() / 3));
        }
    }
    else
    {
        ok = createBuffer(u32(scene.materialVertices.size() * sizeof(MaterialVertex)), D3D11_BIND_VERTEX_BUFFER, scene.materialVertices.data(), &scene.buffer) && createBuffer(u32(scene.indices.size() * sizeof(u16)), D3D11_BIND_INDEX_BUFFER, scene.indices.data(), &scene.indexBuffer);
        if (ok)
        {
            D3D11_BUFFER_DESC desc {}; desc.ByteWidth = scene.instanceCapacity * sizeof(Instance); desc.Usage = D3D11_USAGE_DYNAMIC; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; desc.StructureByteStride = sizeof(Instance);
            ok = SUCCEEDED(RDevice->CreateBuffer(&desc, nullptr, &scene.instanceBuffer));
            if (ok)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC srv {}; srv.Format = DXGI_FORMAT_UNKNOWN; srv.ViewDimension = D3D11_SRV_DIMENSION_BUFFER; srv.Buffer.NumElements = scene.instanceCapacity;
                ok = SUCCEEDED(RDevice->CreateShaderResourceView(scene.instanceBuffer, &srv, &scene.instanceView));
            }
        }
        if (ok)
        {
            D3DVERTEXELEMENT9 declaration[] = {{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0}, {0,12,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_NORMAL,0}, {0,24,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0}, {0,32,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0}, D3DDECL_END()};
            scene.geometry.create(declaration, scene.buffer, scene.indexBuffer); u32 terrain = 0, models = 0, sprites = 0;
            for (auto& p : scene.parts)
            {
                LPCSTR shader = p.data.mode == 0 ? "pf_skybox_ground" : (p.data.mode == 1 ? "pf_skybox_model" : "pf_skybox_billboard");
                p.shader.create(shader, p.data.texture); if (p.data.mode == 0) terrain += p.data.indexCount / 3;
            }
            for (const auto& g : scene.groups) { if (g.mode == 1) models += g.count; if (g.mode == 2) sprites += g.count; }
            Msg("* [PF SKYBOX] GPU v2 ready terrain=%u model_instances=%u billboard_instances=%u batches=%u", terrain, models, sprites, u32(scene.parts.size()));
        }
    }
    if (!ok) { PFSkyboxResetGPU(); scene.gpuFailed = true; Msg("! [PF SKYBOX] GPU resource creation failed; optional scene disabled"); return false; }
    scene.gpuReady = true; return true;
}
bool updateVisibleInstances()
{
    if (scene.version == 1) return true;
    CFrustum frustum; frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_ALL); scene.visibleInstances.clear();
    for (auto& group : scene.groups)
    {
        group.visibleFirst = u32(scene.visibleInstances.size());
        for (u32 j = 0; j < group.count; ++j)
        {
            const auto& i = scene.instances[group.first + j];
            if (group.mode)
            {
                Fvector center; center.set(i.positionHeight.x * scene.scale, (i.positionHeight.y + .5f * i.positionHeight.w) * scene.scale, i.positionHeight.z * scene.scale);
                if (!frustum.testSphere_dirty(center, i.positionHeight.w * scene.scale * 1.5f)) continue;
            }
            scene.visibleInstances.push_back(i);
        }
        group.visibleCount = u32(scene.visibleInstances.size()) - group.visibleFirst;
    }
    if (scene.visibleInstances.empty()) return true;
    D3D11_MAPPED_SUBRESOURCE mapped {};
    if (FAILED(RContext->Map(scene.instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) { scene.gpuFailed = true; Msg("! [PF SKYBOX] instance upload failed; optional scene disabled"); return false; }
    CopyMemory(mapped.pData, scene.visibleInstances.data(), scene.visibleInstances.size() * sizeof(Instance)); RContext->Unmap(scene.instanceBuffer, 0); return true;
}
void bindMatrices(const Fmatrix& current, const Fmatrix& previous, const Fvector& camera, const Fvector& oldCamera)
{
    RCache.set_c("sb_current", current); RCache.set_c("sb_previous", previous);
    RCache.set_c("sb_camera_scale", camera.x, camera.y, camera.z, scene.scale); RCache.set_c("sb_old_camera", oldCamera.x, oldCamera.y, oldCamera.z, scene.scale);
}
void bindMaterialLight(const CEnvDescriptorMixer& weather)
{
    // The same environment/sun inputs as native combine, without a fixed grade.
    const float minamb = .001f;
    RCache.set_c("L_ambient", _max(weather.ambient.x * 2.f, minamb) * ps_r2_sun_lumscale_amb, _max(weather.ambient.y * 2.f, minamb) * ps_r2_sun_lumscale_amb, _max(weather.ambient.z * 2.f, minamb) * ps_r2_sun_lumscale_amb, 0.f);
    RCache.set_c("env_color", (weather.hemi_color.x * 2.f + EPS) * 2.f * ps_r2_sun_lumscale_hemi, (weather.hemi_color.y * 2.f + EPS) * 2.f * ps_r2_sun_lumscale_hemi, (weather.hemi_color.z * 2.f + EPS) * 2.f * ps_r2_sun_lumscale_hemi, weather.weight);
    const light* sun = (light*)RImplementation.Lights.sun_adapted._get(); Fvector direction; Device.mView.transform_dir(direction, sun->direction); direction.normalize();
    RCache.set_c("Ldynamic_color", sun->color.r, sun->color.g, sun->color.b, 0.f); RCache.set_c("Ldynamic_dir", direction.x, direction.y, direction.z, 0.f);
    RCache.set_c("sb_view", Device.mView); Fmatrix inverse; inverse.invert(Device.mView); RCache.set_c("m_invV", inverse);
    RCache.set_c("sb_bounds", scene.bounds.x, scene.bounds.y, 1.f / (scene.bounds.z - scene.bounds.x), 1.f / (scene.bounds.w - scene.bounds.y));
}
}
void PFSkyboxResetGPU()
{
    scene.geometry.destroy(); scene.shader.destroy(); for (auto& p : scene.parts) p.shader.destroy(); scene.depth.destroy(); SRVSManager.SetVSResource(0, nullptr);
    _RELEASE(scene.instanceView); _RELEASE(scene.instanceBuffer); _RELEASE(scene.indexBuffer); _RELEASE(scene.buffer);
    scene.width = scene.height = scene.lastFrame = 0; scene.gpuReady = scene.gpuFailed = false;
}
void PFSkyboxUnload()
{
    PFSkyboxResetGPU(); scene.enabled = false; scene.version = 1; scene.instanceCapacity = 0; scene.vertices.clear(); scene.materialVertices.clear(); scene.indices.clear(); scene.instances.clear(); scene.visibleInstances.clear(); scene.parts.clear(); scene.groups.clear();
}
void PFSkyboxLoad()
{
    PFSkyboxUnload(); string_path configPath;
    if (!FS.exist(configPath, "$game_config$", "pf_skybox.ltx")) return;
    CInifile settings(configPath); const std::string section = levelName();
    if (section.empty() || !settings.section_exist(section.c_str()) || !settings.line_exist(section.c_str(), "enabled") || !settings.r_bool(section.c_str(), "enabled")) return;
    if (!settings.line_exist(section.c_str(), "mesh")) return;
    const LPCSTR asset = settings.r_string(section.c_str(), "mesh"); scene.scale = option(settings, section.c_str(), "scale", 16.f); scene.hemi = option(settings, section.c_str(), "hemi", .8f); scene.exposure = option(settings, section.c_str(), "exposure", 1.f);
    if (!relativeAsset(asset) || !std::isfinite(scene.scale) || scene.scale < 1.f || scene.scale > 256.f || !std::isfinite(scene.hemi) || scene.hemi < 0.f || scene.hemi > 1.f || !std::isfinite(scene.exposure) || scene.exposure < .1f || scene.exposure > 8.f) { Msg("! [PF SKYBOX] invalid config for %s", section.c_str()); return; }
    if (!FS.exist("$game_meshes$", asset)) { Msg("! [PF SKYBOX] mesh missing: %s", asset); return; }
    IReader* file = FS.r_open("$game_meshes$", asset); if (!file) return; bool valid = file->length() >= 16;
    if (valid)
    {
        const u32 magic = file->r_u32(); scene.version = file->r_u32(); const u32 count = file->r_u32(), fourth = file->r_u32();
        valid = magic == 0x42534650u && ((scene.version == 1 && loadLegacy(*file, count, fourth)) || (scene.version == 2 && loadMaterials(*file, count, fourth)));
    }
    FS.r_close(file);
    if (!valid) { PFSkyboxUnload(); Msg("! [PF SKYBOX] invalid PFSB mesh or missing material assets: %s", asset); return; }
    scene.enabled = true; Msg("* [PF SKYBOX] loaded level=%s mesh=%s version=%u scale=%.3f", section.c_str(), asset, scene.version, scene.scale);
}
void PFSkyboxRender(CRenderTarget& target)
{
    if (!scene.enabled || !g_pGamePersistent || !g_pGamePersistent->Environment().CurrentEnv || !createGPU() || !updateVisibleInstances()) return;
    const u32 width = RCache.get_width(), height = RCache.get_height(); if (!width || !height) return;
    if (!scene.depth || scene.width != width || scene.height != height) { scene.depth.destroy(); scene.depth.create("$user$pf_skybox_depth", width, height, DxgiFormat::DXGI_FORMAT_R24G8_TYPELESS); scene.width = width; scene.height = height; }
    if (!scene.depth || !scene.depth->pZRT) return;
    Fmatrix view = Device.mView, oldView = Device.mView_old, current, previous; view.c.div(scene.scale); oldView.c.div(scene.scale); current.mul(Device.mProject, view); previous.mul(Device.mProject_old, oldView);
    Fvector camera = Device.vCameraPosition; camera.div(scene.scale); Fvector oldCamera = scene.previousCamera;
    if (!scene.lastFrame || Device.dwFrame - scene.lastFrame > 5) { previous = current; oldCamera = camera; }
    scene.lastFrame = Device.dwFrame; scene.previousCamera = camera; const auto& weather = *g_pGamePersistent->Environment().CurrentEnv;
    const u32 oldCull = RCache.get_CullMode(); target.u_setrt(target.rt_Generic_0, target.rt_Velocity, 0, scene.depth->pZRT);
    RContext->ClearDepthStencilView(scene.depth->pZRT, D3D_CLEAR_DEPTH | D3D_CLEAR_STENCIL, 1.f, 0); RCache.set_Stencil(FALSE); RCache.set_ColorWriteEnable(); RCache.set_CullMode(CULL_NONE);
    if (scene.version == 1)
    {
        RCache.set_Element(scene.shader->E[0]); RCache.set_Geometry(scene.geometry); bindMatrices(current, previous, camera, oldCamera);
        RCache.set_c("sb_sun_direction", weather.sun_dir.x, weather.sun_dir.y, weather.sun_dir.z, 0.f); RCache.set_c("sb_sun_color", weather.sun_color.x, weather.sun_color.y, weather.sun_color.z, 0.f);
        RCache.set_c("sb_ambient", weather.ambient.x + weather.hemi_color.x * .35f, weather.ambient.y + weather.hemi_color.y * .35f, weather.ambient.z + weather.hemi_color.z * .35f, 0.f);
        RCache.Render(D3DPT_TRIANGLELIST, 0, u32(scene.vertices.size() / 3));
    }
    else
    {
        for (auto& p : scene.parts)
        {
            const auto& group = scene.groups[p.group]; if (!group.visibleCount) continue;
            RCache.set_Element(p.shader->E[0]); RCache.set_Geometry(scene.geometry); RCache.set_CullMode(CULL_NONE); bindMatrices(current, previous, camera, oldCamera); bindMaterialLight(weather);
            RCache.set_c("sb_params", float(p.data.mode), float(group.visibleFirst), scene.hemi, scene.exposure); SRVSManager.SetVSResource(0, p.data.mode ? scene.instanceView : nullptr);
            RCache.RenderInstancedIndexed(D3DPT_TRIANGLELIST, p.data.baseVertex, 0, p.data.vertexCount, p.data.firstIndex, p.data.indexCount / 3, group.visibleCount, 0);
        }
        SRVSManager.SetVSResource(0, nullptr); SRVSManager.Apply();
    }
    RCache.set_CullMode(oldCull); target.u_setrt(target.rt_Generic_0, 0, 0, RDepth);
}
