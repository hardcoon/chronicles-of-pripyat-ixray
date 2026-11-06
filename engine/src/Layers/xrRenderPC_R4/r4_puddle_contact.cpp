#include "stdafx.h"
#include "r4_puddle_contact.h"
#include "r4_puddle_contact_math.h"
#include "../xrRender/SSSSettings.h"
#include "../../xrEngine/Fmesh.h"
#include "../../xrEngine/xrLevel.h"
#include <DirectXTex.h>

extern int get_texture_load_lod(LPCSTR name);

// COP_ENGINE_PATCH: COP-ENG-192 - sample actual terrain UV/masks, never the screen.
namespace
{
struct Image
{
    shared_str name;
    u32 width = 0, height = 0;
    xr_vector<u8> red;
    float sample(float u, float v) const
    {
        return PFPuddleContact::sampleWrap(red.empty() ? nullptr : red.data(), width, height, u, v);
    }
};
struct Vertex { Fvector position, normal; Fvector2 uv; };
struct Attribute { Fvector normal; Fvector2 uv; };
struct Triangle { u32 image; };
struct Mesh
{
    u32 vb, firstVertex, vertexCount, ib, firstIndex, indexCount, image;
    xr_vector<Vertex> vertices;
};
struct ContactState
{
    xr_vector<Mesh> pending;
    xr_vector<Image> images;
    Image noise;
    xr_vector<Fvector> positions;
    xr_vector<CDB::TRI> faces;
    xr_vector<Triangle> triangles;
    xr_vector<Attribute> attributes;
    CDB::MODEL* model = nullptr;
    CDB::COLLIDER query;
    bool ready = false;
} contact;

bool loadImage(Image& image, LPCSTR name)
{
    string_path path;
    xr_strconcat(path, name, ".dds");
    IReader* reader = FS.r_open("$game_textures$", path);
    if (!reader) return false;
    DirectX::ScratchImage source, decoded;
    DirectX::TexMetadata metadata;
    const HRESULT loaded = DirectX::LoadFromDDSMemory(reader->pointer(), reader->length(), DirectX::DDS_FLAGS_NONE, &metadata, source);
    FS.r_close(reader);
    if (FAILED(loaded) || metadata.dimension != DirectX::TEX_DIMENSION_TEXTURE2D || metadata.arraySize != 1 ||
        metadata.width > 8192 || metadata.height > 8192 || !metadata.mipLevels) return false;
    // Match the mip reduction used by dx11Texture.cpp for this exact VFS name.
    size_t mip = 0, width = metadata.width, height = metadata.height;
    int skip = get_texture_load_lod(name);
    while (mip + 1 < metadata.mipLevels && width > 4 && height > 4 && skip-- > 0)
    {
        width /= 2; height /= 2; ++mip;
    }
    const auto* selected = source.GetImage(mip, 0, 0);
    if (!selected) return false;
    if (DirectX::IsCompressed(selected->format))
    {
        if (FAILED(DirectX::Decompress(*selected, DXGI_FORMAT_R8G8B8A8_UNORM, decoded))) return false;
        selected = decoded.GetImage(0, 0, 0);
    }
    else if (selected->format != DXGI_FORMAT_R8G8B8A8_UNORM)
    {
        if (FAILED(DirectX::Convert(*selected, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, 0.f, decoded))) return false;
        selected = decoded.GetImage(0, 0, 0);
    }
    if (!selected) return false;
    image.name = name;
    image.width = u32(selected->width); image.height = u32(selected->height);
    image.red.resize(size_t(image.width) * image.height);
    for (u32 y = 0; y < image.height; ++y)
        for (u32 x = 0; x < image.width; ++x)
            image.red[size_t(y) * image.width + x] = selected->pixels[y * selected->rowPitch + x * 4];
    return true;
}

u32 imageFor(const ref_shader& shader)
{
    if (!shader || !shader->E[0] || !shader->E[0]->flags.bLandscape) return u32(-1);
    for (const auto& pass : shader->E[0]->passes)
    {
        if (!pass->T) continue;
        for (const auto& texture : *pass->T)
        {
            const char* name = texture.second->cName.c_str();
            if (!name || !strstr(name, "_puddles_mask")) continue;
            for (u32 i = 0; i < contact.images.size(); ++i)
                if (contact.images[i].name == name) return i;
            Image image;
            if (!loadImage(image, name))
            {
                Msg("! [SSS contact] mask unavailable: %s; footsteps remain dry", name);
                return u32(-1);
            }
            contact.images.push_back(std::move(image));
            return u32(contact.images.size() - 1);
        }
    }
    // Native white fallback means 1 - mask == 0, so this terrain has no puddles.
    return u32(-1);
}
const D3DVERTEXELEMENT9* element(const D3DVERTEXELEMENT9* declaration, u8 usage)
{
    for (u32 i = 0; i < MAXD3DDECLLENGTH && declaration[i].Stream != 0xff; ++i)
        if (declaration[i].Stream == 0 && declaration[i].Usage == usage && declaration[i].UsageIndex == 0)
            return declaration + i;
    return nullptr;
}
bool packedColor(const D3DVERTEXELEMENT9* value)
{
    return value && (value->Type == D3DDECLTYPE_D3DCOLOR || value->Type == D3DDECLTYPE_UBYTE4N);
}
}

void SSSPuddleContactReset()
{
    // Called even on an incomplete load, before native level_Unload early exits.
    contact.ready = false;
    xr_delete(contact.model);
    contact.pending.clear(); contact.images.clear();
    contact.positions.clear(); contact.faces.clear(); contact.triangles.clear();
    contact.attributes.clear();
    contact.noise = Image();
    contact.query.r_free();
}

void SSSPuddleContactBegin(IReader* level, const xr_vector<ref_shader>& shaders)
{
    SSSPuddleContactReset();
    if (!level || g_dedicated_server || !loadImage(contact.noise, "pf_sss\\fx\\puddles_perlin")) return;
    IReader* visuals = level->open_chunk(fsL_VISUALS);
    if (!visuals) return;
    xr_vector<u32> materials(shaders.size(), u32(-2));
    for (u32 index = 0; ; ++index)
    {
        IReader* data = visuals->open_chunk(index);
        if (!data) break;
        ogf_header header;
        if (data->r_chunk_safe(OGF_HEADER, &header, sizeof(header)) && header.format_version == xrOGF_FormatVersion &&
            (header.type == MT_NORMAL || header.type == MT_PROGRESSIVE) && header.shader_id < shaders.size() &&
            data->find_chunk(OGF_GCONTAINER) >= 24)
        {
            u32& image = materials[header.shader_id];
            if (image == u32(-2)) image = imageFor(shaders[header.shader_id]);
            if (image != u32(-1))
            {
                Mesh mesh;
                mesh.vb = data->r_u32(); mesh.firstVertex = data->r_u32(); mesh.vertexCount = data->r_u32();
                mesh.ib = data->r_u32(); mesh.firstIndex = data->r_u32(); mesh.indexCount = data->r_u32(); mesh.image = image;
                // FProgressive draws one sliding window, not its full concatenated LOD stream.
                // At a first-person foot contact use window zero (the full detail surface).
                if (header.type == MT_PROGRESSIVE)
                {
                    bool valid = false;
                    if (data->find_chunk(OGF_SWIDATA) >= 28)
                    {
                        data->advance(16);
                        const u32 windows = data->r_u32();
                        FSlideWindow window;
                        data->r(&window, sizeof(window));
                        if (windows && window.offset <= mesh.indexCount && u32(window.num_tris) * 3 <= mesh.indexCount - window.offset &&
                            window.num_verts <= mesh.vertexCount)
                        {
                            mesh.firstIndex += window.offset; mesh.indexCount = u32(window.num_tris) * 3;
                            mesh.vertexCount = window.num_verts; valid = true;
                        }
                    }
                    if (!valid) mesh.indexCount = 0;
                }
                if (mesh.vertexCount && mesh.vertexCount <= 65536 && mesh.indexCount >= 3 && mesh.indexCount % 3 == 0)
                    contact.pending.push_back(std::move(mesh));
            }
        }
        data->close();
    }
    visuals->close();
}

void SSSPuddleContactVertices(u32 id, const void* bytes, u32 count, u32 stride, const D3DVERTEXELEMENT9* declaration)
{
    const auto* p = element(declaration, D3DDECLUSAGE_POSITION);
    const auto* n = element(declaration, D3DDECLUSAGE_NORMAL);
    const auto* t = element(declaration, D3DDECLUSAGE_TANGENT);
    const auto* b = element(declaration, D3DDECLUSAGE_BINORMAL);
    const auto* uv = element(declaration, D3DDECLUSAGE_TEXCOORD);
    if (!p || p->Type != D3DDECLTYPE_FLOAT3 || !packedColor(n) || !packedColor(t) || !packedColor(b) ||
        !uv || uv->Type != D3DDECLTYPE_SHORT2 || p->Offset + 12 > stride || n->Offset + 4 > stride ||
        t->Offset + 4 > stride || b->Offset + 4 > stride || uv->Offset + 4 > stride) return;
    for (auto& mesh : contact.pending)
    {
        if (mesh.vb != id || mesh.firstVertex > count || mesh.vertexCount > count - mesh.firstVertex) continue;
        mesh.vertices.resize(mesh.vertexCount);
        for (u32 i = 0; i < mesh.vertexCount; ++i)
        {
            const auto* source = static_cast<const u8*>(bytes) + size_t(mesh.firstVertex + i) * stride;
            Vertex& vertex = mesh.vertices[i];
            std::memcpy(&vertex.position, source + p->Offset, 12);
            const u8* normal = source + n->Offset;
            // D3DCOLOR is BGRA in memory; the VS uses unpack_bx4(unpack_D3DCOLOR).
            vertex.normal.set(float(normal[2]) * (4.f / 255.f) - 2.f, float(normal[1]) * (4.f / 255.f) - 2.f, float(normal[0]) * (4.f / 255.f) - 2.f);
            s16 packed[2]; std::memcpy(packed, source + uv->Offset, sizeof(packed));
            vertex.uv.set(PFPuddleContact::unpackUV(packed[0], source[t->Offset + 3]), PFPuddleContact::unpackUV(packed[1], source[b->Offset + 3]));
        }
    }
}

void SSSPuddleContactIndices(u32 id, const u16* indices, u32 count)
{
    for (auto& mesh : contact.pending)
    {
        if (mesh.ib != id || mesh.vertices.empty() || mesh.firstIndex > count || mesh.indexCount > count - mesh.firstIndex) continue;
        const u32 first = u32(contact.positions.size());
        for (const Vertex& vertex : mesh.vertices)
        {
            contact.positions.push_back(vertex.position);
            contact.attributes.push_back({vertex.normal, vertex.uv});
        }
        for (u32 i = 0; i < mesh.indexCount; i += 3)
        {
            const u16* face = indices + mesh.firstIndex + i;
            if (face[0] >= mesh.vertices.size() || face[1] >= mesh.vertices.size() || face[2] >= mesh.vertices.size()) continue;
            CDB::TRI triangle; triangle.dummy = 0;
            for (u32 k = 0; k < 3; ++k)
                triangle.verts[k] = first + face[k];
            contact.faces.push_back(triangle); contact.triangles.push_back({mesh.image});
        }
        mesh.vertices.clear();
    }
}

void SSSPuddleContactFinish()
{
    xr_vector<Mesh>().swap(contact.pending);
    if (contact.faces.size() < 2) { SSSPuddleContactReset(); return; }
    contact.model = new CDB::MODEL();
    contact.model->build(contact.positions.data(), contact.positions.size(), contact.faces.data(), contact.faces.size());
    contact.model->syncronize();
    xr_vector<Fvector>().swap(contact.positions);
    xr_vector<CDB::TRI>().swap(contact.faces);
    contact.query.ray_options(CDB::OPT_ONLYNEAREST | CDB::OPT_CULL);
    contact.ready = true;
    const u32 memory = contact.model->memory() + u32(contact.attributes.capacity() * sizeof(Attribute) + contact.triangles.capacity() * sizeof(Triangle));
    Msg("[SSS contact] terrain ready: %u triangles, %u vertices, %u masks, %u KB; CPU UV probe", u32(contact.triangles.size()), u32(contact.attributes.size()), u32(contact.images.size()), memory / 1024);
}

extern float ssfx_update_puddle_wetness();
extern "C" __declspec(dllexport) float __cdecl xrSSSPuddleContact(float x, float y, float z)
{
    if (!contact.ready || !ssfx_puddles_active() || !PFPuddleContact::finite(x) || !PFPuddleContact::finite(y) || !PFPuddleContact::finite(z)) return 0.f;
    // The gameplay ray already found the real floor: do not reach terrain under a platform.
    Fvector start; start.set(x, y + .01f, z);
    Fvector down; down.set(0.f, -1.f, 0.f);
    contact.query.ray_query(contact.model, start, down, .02f);
    if (!contact.query.r_count()) return 0.f;
    const auto& hit = *contact.query.r_begin();
    if (hit.id < 0 || size_t(hit.id) >= contact.triangles.size()) return 0.f;
    const Triangle& triangle = contact.triangles[hit.id];
    const CDB::TRI& face = contact.model->get_tris()[hit.id];
    const float weights[] = {1.f - hit.u - hit.v, hit.u, hit.v};
    Fvector normal; normal.set(0.f, 0.f, 0.f);
    Fvector2 uv; uv.set(0.f, 0.f);
    for (u32 k = 0; k < 3; ++k)
    {
        const Attribute& vertex = contact.attributes[face.verts[k]];
        normal.mad(vertex.normal, weights[k]);
        uv.x += vertex.uv.x * weights[k]; uv.y += vertex.uv.y * weights[k];
    }
    if (normal.square_magnitude() < .00001f) return 0.f;
    normal.normalize();
    float wetness = ssfx_update_puddle_wetness();
    wetness = ssfx_option(SSS_puddles_always) > .5f ? 1.f : _max(ssfx_option(SSS_puddles_min_wetness), PFPuddleContact::saturate(wetness));
    const float scale = 15.f / _max(.1f, ssfx_option(SSS_puddles_global_size));
    return PFPuddleContact::coverage(contact.images[triangle.image].sample(uv.x, uv.y), contact.noise.sample(uv.x * scale, uv.y * scale), normal.y,
        wetness, ssfx_option(SSS_puddles_size), ssfx_option(SSS_puddles_border));
}
