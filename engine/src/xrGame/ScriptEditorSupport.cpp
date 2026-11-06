#include "StdAfx.h"
#include "pch_script.h"
#include "ScriptEditorSupport.h"
#include "ScriptOgfPreview.h"
#include "Level.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "../xrEngine/Render.h"
#include "../xrEngine/xr_input.h"
#include "../Include/xrRender/ScriptDebugRender.h"
#include <memory>
#include <cmath>
#include <cstring>
#include <algorithm>

// COP_ENGINE_PATCH: COP-ENG-HF-001 - engine-neutral primitives and scoped input.
namespace
{
struct PreviewPoint { Fvector position; Fvector2 uv; };
struct PreviewGroup { shared_str texture; xr_vector<PreviewPoint> points; };
struct Primitive
{
    int type = 0;
    bool alive = true;
    bool visible = false;
    bool cameraRelative = false;
    Fcolor color;
    Fvector pointA, pointB;
    Fvector previewExtent;
    Fmatrix transform;
    xr_vector<PreviewGroup> groups;
    explicit Primitive(int kind) : type(kind)
    {
        color.set(1.f, 1.f, 1.f, 1.f);
        pointA.set(0.f, 0.f, 0.f);
        pointB.set(0.f, 0.f, 0.f);
        previewExtent.set(0.f, 0.f, 0.f);
        transform.identity();
    }
};

// Lua receives a value handle with shared ownership, not a dangling pointer
// into the registry. Removal invalidates the primitive even if Lua retains it.
struct DebugObject
{
    std::shared_ptr<Primitive> data;
    DebugObject() = default;
    explicit DebugObject(std::shared_ptr<Primitive> value) : data(std::move(value)) {}
    bool GetVisible() const { return data && data->alive && data->visible; }
    void SetVisible(bool value) { if (data && data->alive) data->visible = value; }
    Fcolor GetColor() const { Fcolor c; c.set(0.f, 0.f, 0.f, 0.f); return data ? data->color : c; }
    void SetColor(const Fcolor& value) { if (data && data->alive) data->color = value; }
    DebugObject CastLine() const { return data && data->type == 0 ? *this : DebugObject(); }
    DebugObject CastSphere() const { return data && data->type == 1 ? *this : DebugObject(); }
    bool Valid() const { return data && data->alive; }
    Fvector GetExtent() const { Fvector v; v.set(0.f, 0.f, 0.f); return data ? data->previewExtent : v; }
    Fvector GetA() const { Fvector v; v.set(0.f, 0.f, 0.f); return data ? data->pointA : v; }
    Fvector GetB() const { Fvector v; v.set(0.f, 0.f, 0.f); return data ? data->pointB : v; }
    void SetA(const Fvector& value) { if (data && data->alive) data->pointA = value; }
    void SetB(const Fvector& value) { if (data && data->alive) data->pointB = value; }
    Fmatrix GetMatrix() const { Fmatrix m; m.identity(); return data ? data->transform : m; }
    void SetMatrix(const Fmatrix& value) { if (data && data->alive) data->transform = value; }
};
xr_map<shared_str, std::shared_ptr<Primitive>> primitives;
xr_vector<ScriptDebugVertex> vertices;
xr_vector<ScriptPreviewVertex> modelVertices;
luabind::functor<bool> inputHandler;
u32 inputToken = 0;
u32 nextToken = 0;

bool EditorAvailable() { return ScriptDRender != nullptr; }
int EditorApiVersion() { return 1; }
DebugObject EditorAdd(LPCSTR name, int type)
{
    if (!g_pGameLevel || !name || !*name || (type != 0 && type != 1)) return DebugObject();
    const shared_str key(name);
    auto found = primitives.find(key);
    if (found != primitives.end())
    {
        if (found->second->type == type) return DebugObject(found->second);
        found->second->alive = false;
        primitives.erase(found);
    }
    if (primitives.size() >= 4096) return DebugObject();
    auto value = std::make_shared<Primitive>(type);
    primitives.emplace(key, value);
    return DebugObject(std::move(value));
}
DebugObject EditorGet(LPCSTR name)
{
    if (!name) return DebugObject();
    auto found = primitives.find(shared_str(name));
    return found == primitives.end() ? DebugObject() : DebugObject(found->second);
}
// Immutable triangle assets are loaded once per session, never per frame.
// Validate every size before reading: malformed addon data must not hit VERIFY.
bool RelativeAsset(LPCSTR path)
{
    return path && *path && xr_strlen(path) < 240 && !strstr(path, "..") &&
        !strchr(path, ':') && path[0] != '/' && path[0] != '\\';
}
struct PreviewReader
{
    const u8* data;
    size_t left;
    bool Read(void* output, size_t count)
    {
        if (count > left) return false;
        memcpy(output, data, count); data += count; left -= count; return true;
    }
};
DebugObject EditorAddModel(LPCSTR name, LPCSTR path)
{
    if (!g_pGameLevel || !name || !*name || !RelativeAsset(path) || primitives.size() >= 4096)
        return DebugObject();
    if (!FS.exist("$game_meshes$", path)) return DebugObject();
    IReader* file = FS.r_open("$game_meshes$", path);
    if (!file) return DebugObject();
    if (file->length() > 64u * 1024u * 1024u) { FS.r_close(file); return DebugObject(); }
    PreviewReader reader{static_cast<const u8*>(file->pointer()), static_cast<size_t>(file->length())};
    char magic[4]; u32 version = 0, groupCount = 0;
    bool ok = reader.Read(magic, 4) && memcmp(magic, "HFPM", 4) == 0 &&
        reader.Read(&version, 4) && version == 1 && reader.Read(&groupCount, 4) && groupCount && groupCount <= 128;
    auto value = std::make_shared<Primitive>(2);
    u32 totalCount = 0;
    for (u32 group = 0; ok && group < groupCount; ++group)
    {
        u16 textureLength = 0; u32 count = 0; char texture[256] = {};
        ok = reader.Read(&textureLength, 2) && textureLength && textureLength < sizeof(texture) &&
            reader.Read(texture, textureLength) && !memchr(texture, 0, textureLength) && RelativeAsset(texture) &&
            reader.Read(&count, 4) && count && count % 3 == 0 && count <= 300000 &&
            totalCount + count <= 1000000 && reader.left >= size_t(count) * 20;
        if (!ok) break;
        PreviewGroup output;
        output.texture = texture;
        output.points.reserve(count);
        for (u32 i = 0; ok && i < count; ++i)
        {
            PreviewPoint point;
            ok = reader.Read(&point.position, 12) && reader.Read(&point.uv, 8) &&
                std::isfinite(point.position.x) && std::isfinite(point.position.y) && std::isfinite(point.position.z) &&
                std::isfinite(point.uv.x) && std::isfinite(point.uv.y) &&
                fabsf(point.position.x) < 10000.f && fabsf(point.position.y) < 10000.f && fabsf(point.position.z) < 10000.f;
            if (ok) output.points.push_back(point);
        }
        totalCount += count;
        if (ok) value->groups.push_back(std::move(output));
    }
    ok = ok && reader.left == 0;
    FS.r_close(file);
    if (!ok) { Msg("! [runtime_editor] invalid preview mesh: %s", path); return DebugObject(); }
    const shared_str key(name);
    auto old = primitives.find(key);
    if (old != primitives.end()) { old->second->alive = false; primitives.erase(old); }
    primitives.emplace(key, value);
    return DebugObject(std::move(value));
}
// COP_ENGINE_PATCH: COP-ENG-113 - read the selected world OGF directly from VFS.
DebugObject EditorAddOgfModel(LPCSTR name, LPCSTR path)
{
    if (!g_pGameLevel || !name || !*name || !RelativeAsset(path) || primitives.size() >= 4096)
        return DebugObject();
    std::string asset(path);
    if (asset.size() < 4 || asset.substr(asset.size() - 4) != ".ogf") asset += ".ogf";
    if (!FS.exist("$game_meshes$", asset.c_str())) return DebugObject();
    IReader* file = FS.r_open("$game_meshes$", asset.c_str());
    if (!file) return DebugObject();
    ScriptOgfPreview::Mesh mesh;
    const bool ok = ScriptOgfPreview::load(file->pointer(), file->length(), mesh);
    FS.r_close(file);
    if (!ok) return DebugObject();
    auto value = std::make_shared<Primitive>(2);
    value->previewExtent.set(mesh.extent[0], mesh.extent[1], mesh.extent[2]);
    for (const auto& source : mesh.groups)
    {
        if (!RelativeAsset(source.texture.c_str())) return DebugObject();
        PreviewGroup group;
        group.texture = source.texture.c_str();
        group.points.reserve(source.points.size());
        for (const auto& p : source.points)
        {
            PreviewPoint point;
            point.position.set(p.x, p.y, p.z); point.uv.set(p.u, p.v);
            group.points.push_back(point);
        }
        value->groups.push_back(std::move(group));
    }
    const shared_str key(name);
    auto old = primitives.find(key);
    if (old != primitives.end()) { old->second->alive = false; primitives.erase(old); }
    primitives.emplace(key, value);
    return DebugObject(std::move(value));
}
// COP_ENGINE_PATCH: COP-ENG-114 - UI OGF vertices stay in camera coordinates.
// Alpha byte 0 is a private vertex protocol tag (world alpha-zero is skipped).
// It selects m_P in script_preview_model.vs; no renderer ABI change is needed.
DebugObject EditorAddUiOgfModel(LPCSTR name, LPCSTR path)
{
    DebugObject value = EditorAddOgfModel(name, path);
    if (value.Valid()) value.data->cameraRelative = true;
    return value;
}
void EditorRemove(LPCSTR name)
{
    if (!name) return;
    auto found = primitives.find(shared_str(name));
    if (found == primitives.end()) return;
    found->second->alive = false;
    primitives.erase(found);
    if (primitives.empty() && ScriptDRender) ScriptDRender->ClearModels();
}
void EditorClearOwner(LPCSTR prefix)
{
    if (!prefix || !*prefix) return;
    const size_t length = xr_strlen(prefix);
    for (auto it = primitives.begin(); it != primitives.end();)
    {
        if (strncmp(it->first.c_str(), prefix, length) == 0)
        {
            it->second->alive = false;
            it = primitives.erase(it);
        }
        else ++it;
    }
    if (primitives.empty() && ScriptDRender) ScriptDRender->ClearModels();
}
u32 EditorCount() { return static_cast<u32>(primitives.size()); }
u32 EditorCapture(const luabind::functor<bool>& handler)
{
    if (!g_pGameLevel || inputToken || !handler.is_valid()) return 0;
    if (++nextToken == 0) ++nextToken;
    inputToken = nextToken;
    inputHandler = handler;
    return inputToken;
}
bool EditorReleaseCapture(u32 token)
{
    if (!token || token != inputToken) return false;
    inputHandler = luabind::functor<bool>();
    inputToken = 0;
    return true;
}
bool EditorKeyDown(int key) { return pInput && pInput->iGetAsyncKeyState(key); }
u32 EditorCaptureOwner() { return inputToken; }
bool EditorFinite(const Fvector& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
// COP_ENGINE_PATCH: COP-ENG-HF-005 - animation relative to a fixed world frame.
// Keep cancellation separate from the effector: AddCamEffector is deferred,
// so removing an active effector alone cannot cancel a queued camera start.
struct AnchoredCameraState
{
    bool active = true;
    Fvector position, direction, up;
};
xr_map<int, std::shared_ptr<AnchoredCameraState>> anchoredCameras;
class AnchoredCameraEffector final : public CAnimatorCamEffectorScriptCB
{
    std::shared_ptr<AnchoredCameraState> anchor;
public:
    AnchoredCameraEffector(LPCSTR callback, std::shared_ptr<AnchoredCameraState> state)
        : CAnimatorCamEffectorScriptCB(callback), anchor(std::move(state)) {}
    BOOL Valid() override { return anchor->active && CAnimatorCamEffectorScriptCB::Valid(); }
    bool AbsolutePositioning() override { return anchor->active; }
    BOOL AllowProcessingIfInvalid() override { return FALSE; }
    BOOL ProcessCam(SCamEffectorInfo& info) override
    {
        if (!anchor->active) return FALSE;
        info.p = anchor->position;
        info.d = anchor->direction;
        info.n = anchor->up;
        // Process after ordinary actor effects, but compose the existing local
        // animation with this fixed frame (m_bAbsolutePositioning stays false).
        return CAnimatorCamEffector::ProcessCam(info);
    }
};
void ClearAnchoredCameras()
{
    for (auto& entry : anchoredCameras) entry.second->active = false;
    anchoredCameras.clear();
}
void RemoveAnchoredCamera(int id)
{
    auto found = anchoredCameras.find(id);
    if (found == anchoredCameras.end()) return;
    found->second->active = false;
    anchoredCameras.erase(found);
    if (g_pGameLevel && Actor()) Actor()->Cameras().RemoveCamEffector((ECamEffectorType)id);
}
float AddAnchoredCamera(LPCSTR filename, int id, bool cyclic, LPCSTR callback,
    const Fvector& position, const Fvector& direction)
{
    if (!g_pGameLevel || !Actor() || !RelativeAsset(filename) ||
        !EditorFinite(position) || !EditorFinite(direction)) return 0.f;
    const float length = direction.magnitude();
    if (!std::isfinite(length) || length < EPS_S) return 0.f;
    if (!FS.exist("$level$", filename) && !FS.exist("$game_anims$", filename)) return 0.f;
    auto state = std::make_shared<AnchoredCameraState>();
    state->position = position;
    state->direction.set(direction).div(length);
    state->up.set(0.f, 1.f, 0.f);
    if (fabsf(state->direction.y) > .99f) state->up.set(0.f, 0.f, 1.f);
    Fvector right;
    right.crossproduct(state->up, state->direction).normalize();
    state->up.crossproduct(state->direction, right).normalize();
    RemoveAnchoredCamera(id);
    auto* effector = new AnchoredCameraEffector(callback ? callback : "", state);
    effector->SetType((ECamEffectorType)id);
    effector->SetCyclic(cyclic);
    effector->Start(filename);
    anchoredCameras[id] = state;
    Actor()->Cameras().AddCamEffector(effector);
    return effector->GetAnimatorLength();
}
// End COP-ENG-HF-005.
void EditorLine(const Fvector& a, const Fvector& b, u32 color)
{
    if (!EditorFinite(a) || !EditorFinite(b)) return;
    vertices.push_back({a, color});
    vertices.push_back({b, color});
}
}

void ScriptEditorSupport::Render()
{
    if (!ScriptDRender || primitives.empty()) return;
    vertices.clear();
    for (const auto& entry : primitives)
    {
        const Primitive& p = *entry.second;
        const u32 packedColor = p.color.get();
        if (!p.alive || !p.visible || !(packedColor & 0xff000000)) continue;
        const u32 color = p.cameraRelative ? (packedColor & 0x00ffffff) : packedColor;
        if (p.type == 2)
        {
            for (const auto& group : p.groups)
            {
                modelVertices.resize(group.points.size());
                for (size_t i = 0; i < group.points.size(); ++i)
                {
                    p.transform.transform_tiny(modelVertices[i].position, group.points[i].position);
                    modelVertices[i].uv = group.points[i].uv;
                    modelVertices[i].color = color;
                }
                struct Triangle { u32 index; float distance; };
                xr_vector<Triangle> order;
                order.reserve(modelVertices.size() / 3);
                for (u32 i = 0; i < modelVertices.size(); i += 3)
                {
                    Fvector centre;
                    centre.add(modelVertices[i].position, modelVertices[i+1].position).add(modelVertices[i+2].position).div(3.f);
                    order.push_back({i, p.cameraRelative ? centre.z : centre.distance_to_sqr(Device.vCameraPosition)});
                }
                std::sort(order.begin(), order.end(), [&p](const Triangle& a, const Triangle& b)
                {
                    if (p.cameraRelative && a.distance == b.distance) return a.index < b.index;
                    return a.distance > b.distance;
                });
                xr_vector<ScriptPreviewVertex> sorted;
                sorted.reserve(modelVertices.size());
                for (const auto& triangle : order)
                    for (u32 i = 0; i < 3; ++i) sorted.push_back(modelVertices[triangle.index+i]);
                ScriptDRender->DrawTriangles(sorted.data(), static_cast<u32>(sorted.size()), group.texture.c_str());
            }
        }
        else if (p.type == 0) EditorLine(p.pointA, p.pointB, color);
        else
        {
            constexpr int segments = 24;
            for (int plane = 0; plane < 3; ++plane)
                for (int i = 0; i < segments; ++i)
                {
                    const float a = PI_MUL_2 * float(i) / segments;
                    const float b = PI_MUL_2 * float(i + 1) / segments;
                    Fvector va, vb, wa, wb;
                    if (plane == 0) { va.set(cosf(a), sinf(a), 0.f); vb.set(cosf(b), sinf(b), 0.f); }
                    else if (plane == 1) { va.set(cosf(a), 0.f, sinf(a)); vb.set(cosf(b), 0.f, sinf(b)); }
                    else { va.set(0.f, cosf(a), sinf(a)); vb.set(0.f, cosf(b), sinf(b)); }
                    p.transform.transform_tiny(wa, va);
                    p.transform.transform_tiny(wb, vb);
                    EditorLine(wa, wb, color);
                }
        }
    }
    if (!vertices.empty()) ScriptDRender->DrawLines(vertices.data(), static_cast<u32>(vertices.size()));
}

bool ScriptEditorSupport::Input(int event, int key, int action, int dx, int dy)
{
    if (!inputToken || !inputHandler.is_valid()) return false;
    // The handler may release its own capture during this call.
    const auto handler = inputHandler;
    const u32 token = inputToken;
    lua_State* state = handler.lua_state();
    const int top = lua_gettop(state);
    handler.pushvalue();
    lua_pushinteger(state, event);
    lua_pushinteger(state, key);
    lua_pushinteger(state, action);
    lua_pushinteger(state, dx);
    lua_pushinteger(state, dy);
    // LUABIND_NO_EXCEPTIONS uses the engine's fatal error callback. Call Lua
    // directly through pcall so a broken editor can still release input.
    if (lua_pcall(state, 5, 1, 0) != 0)
    {
        Msg("! [runtime_editor] input handler failed: %s", lua_tostring(state, -1));
        lua_settop(state, top);
        EditorReleaseCapture(token);
        return true;
    }
    const bool consumed = lua_toboolean(state, -1) != 0;
    lua_settop(state, top);
    return consumed;
}

void ScriptEditorSupport::Clear()
{
    ClearAnchoredCameras();
    if (inputToken) EditorReleaseCapture(inputToken);
    for (auto& entry : primitives) entry.second->alive = false;
    primitives.clear();
    vertices.clear();
    modelVertices.clear();
    if (ScriptDRender) ScriptDRender->ClearModels();
}

void ScriptEditorSupport::Register(lua_State* state)
{
    using namespace luabind;
    module(state)
    [
        class_<DebugObject>("DBG_ScriptObject")
            .enum_("types")[value("line", 0), value("sphere", 1)]
            .property("visible", &DebugObject::GetVisible, &DebugObject::SetVisible)
            .property("color", &DebugObject::GetColor, &DebugObject::SetColor)
            .property("point_a", &DebugObject::GetA, &DebugObject::SetA)
            .property("point_b", &DebugObject::GetB, &DebugObject::SetB)
            .property("matrix", &DebugObject::GetMatrix, &DebugObject::SetMatrix)
            .property("extent", &DebugObject::GetExtent)
            .def("cast_dbg_line", &DebugObject::CastLine)
            .def("cast_dbg_sphere", &DebugObject::CastSphere)
            .def("valid", &DebugObject::Valid)
    ];
    module(state, "debug_render")
    [
        def("add_object", &EditorAdd), def("get_object", &EditorGet), def("remove_object", &EditorRemove),
        def("clear_owner", &EditorClearOwner), def("object_count", &EditorCount),
        def("add_model", &EditorAddModel),
        def("add_ogf_model", &EditorAddOgfModel),
        def("add_ui_ogf_model", &EditorAddUiOgfModel),
        def("available", &EditorAvailable), def("api_version", &EditorApiVersion)
    ];
    module(state, "level")
    [
        def("capture_editor_input", &EditorCapture), def("release_editor_input", &EditorReleaseCapture),
        def("editor_key_down", &EditorKeyDown), def("editor_input_owner", &EditorCaptureOwner),
        def("add_cam_effector_at", &AddAnchoredCamera), def("remove_cam_effector_at", &RemoveAnchoredCamera)
    ];
}
