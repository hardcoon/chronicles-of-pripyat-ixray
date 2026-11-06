#pragma once
// COP_ENGINE_PATCH: COP-ENG-113 - bounded OGF reader for transient UI previews.
// Uses existing world geometry in memory; no derived mesh files or ALife object.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace ScriptOgfPreview
{
struct Point { float x, y, z, u, v; };
struct Group { std::string texture; std::vector<Point> points; };
struct Mesh
{
    std::vector<Group> groups;
    std::array<float, 3> extent{};
    size_t vertices = 0, repairedUVs = 0;
};
struct Bytes
{
    const uint8_t* data = nullptr;
    size_t size = 0;
    template<class T> bool at(size_t offset, T& value) const
    {
        if (offset > size || sizeof(T) > size - offset) return false;
        std::memcpy(&value, data + offset, sizeof(T));
        return true;
    }
};
inline bool chunks(Bytes input, std::vector<std::pair<uint32_t, Bytes>>& result)
{
    size_t offset = 0;
    while (offset < input.size)
    {
        uint32_t id = 0, count = 0;
        if (!input.at(offset, id) || !input.at(offset + 4, count) ||
            (id & 0x80000000) || count > input.size - offset - 8) return false;
        result.emplace_back(id, Bytes{input.data + offset + 8, count});
        if (result.size() > 1024) return false;
        offset += 8 + count;
    }
    return true;
}
inline bool read(Bytes input, Mesh& output, unsigned depth = 0)
{
    if (depth > 16) return false;
    std::vector<std::pair<uint32_t, Bytes>> parts;
    if (!chunks(input, parts)) return false;
    auto find = [&](uint32_t id)
    {
        Bytes value;
        for (const auto& p : parts) if (p.first == id) value = p.second;
        return value;
    };
    const Bytes header = find(1), children = find(9);
    if (header.size < 44 || header.data[0] != 4) return false;
    if (children.data)
    {
        std::vector<std::pair<uint32_t, Bytes>> entries;
        if (!chunks(children, entries) || entries.empty()) return false;
        for (const auto& p : entries) if (!read(p.second, output, depth + 1)) return false;
        return true;
    }
    const Bytes texture = find(2), vertices = find(3), indices = find(4), swi = find(6);
    if (!vertices.data && !indices.data) return true;
    if (!texture.data || !vertices.data || !indices.data) return false;
    const void* end = std::memchr(texture.data, 0, texture.size);
    if (!end) return false;
    const size_t length = static_cast<const uint8_t*>(end) - texture.data;
    // One stock scope child has an empty texture. It has no textured surface.
    if (!length) return true;
    if (length >= 240 || output.groups.size() >= 128) return false;
    Group group;
    group.texture.assign(reinterpret_cast<const char*>(texture.data), length);
    uint32_t kind = 0, vertexCount = 0, indexCount = 0;
    if (!vertices.at(0, kind) || !vertices.at(4, vertexCount) || !indices.at(0, indexCount)) return false;
    size_t stride = 0, position = 0, uv = 0;
    // xrEngine/bone.h, pack(2): both current IDs and original FVF aliases.
    switch (kind)
    {
    case 1: case 0x12071980: stride = 60; position = 0; uv = 48; break;
    case 2: case 2 * 0x12071980: stride = 64; position = 4; uv = 56; break;
    case 3: case 4 * 0x12071980: stride = 70; position = 6; uv = 62; break;
    case 4: case 5 * 0x12071980: stride = 76; position = 8; uv = 68; break;
    default: return false;
    }
    if (!vertexCount || vertexCount > 1000000 || vertices.size != 8 + size_t(vertexCount) * stride ||
        indexCount % 3 || indices.size != 4 + size_t(indexCount) * 2) return false;
    uint32_t first = 0, count = indexCount;
    if (swi.data)
    {
        uint32_t windows = 0; uint16_t triangles = 0;
        if (!swi.at(16, windows) || !windows || swi.size != 20 + size_t(windows) * 8 ||
            !swi.at(20, first) || !swi.at(24, triangles)) return false;
        count = uint32_t(triangles) * 3; // Highest-detail sliding window, as in FProgressive.
    }
    if (!count || count > 300000 || first > indexCount || count > indexCount - first ||
        output.vertices + count > 1000000) return false;
    group.points.reserve(count);
    for (uint32_t n = 0; n < count; ++n)
    {
        uint16_t index = 0;
        if (!indices.at(4 + size_t(first + n) * 2, index) || index >= vertexCount) return false;
        const size_t offset = 8 + size_t(index) * stride;
        Point p{};
        if (!vertices.at(offset + position, p.x) || !vertices.at(offset + position + 4, p.y) ||
            !vertices.at(offset + position + 8, p.z) || !vertices.at(offset + uv, p.u) ||
            !vertices.at(offset + uv + 4, p.v) || !std::isfinite(p.x) || !std::isfinite(p.y) ||
            !std::isfinite(p.z) || std::fabs(p.x) >= 10000 || std::fabs(p.y) >= 10000 || std::fabs(p.z) >= 10000) return false;
        // A few stock world models contain NaN UVs. Keep their geometry finite
        // in this transient preview without modifying the original OGF.
        if (!std::isfinite(p.u)) { p.u = 0; ++output.repairedUVs; }
        if (!std::isfinite(p.v)) { p.v = 0; ++output.repairedUVs; }
        group.points.push_back(p);
    }
    output.vertices += count;
    output.groups.push_back(std::move(group));
    return true;
}
inline bool load(const void* data, size_t size, Mesh& result)
{
    result = Mesh{};
    if (!data || !size || size > 64u * 1024u * 1024u) return false;
    Mesh value;
    if (!read(Bytes{static_cast<const uint8_t*>(data), size}, value) || value.groups.empty()) return false;
    std::array<float, 3> lo, hi;
    lo.fill(std::numeric_limits<float>::max()); hi.fill(-std::numeric_limits<float>::max());
    for (const auto& g : value.groups) for (const auto& p : g.points)
    {
        const float xyz[] = {p.x, p.y, p.z};
        for (size_t a = 0; a < 3; ++a) { lo[a] = std::min(lo[a], xyz[a]); hi[a] = std::max(hi[a], xyz[a]); }
    }
    const float scale = std::max({hi[0]-lo[0], hi[1]-lo[1], hi[2]-lo[2]});
    if (!std::isfinite(scale) || scale <= .00001f) return false;
    const float x = (lo[0]+hi[0])*.5f, y = (lo[1]+hi[1])*.5f, z = (lo[2]+hi[2])*.5f;
    for (auto& g : value.groups) for (auto& p : g.points)
    { p.x = (p.x-x)/scale; p.y = (p.y-y)/scale; p.z = (p.z-z)/scale; }
    for (size_t a = 0; a < 3; ++a) value.extent[a] = (hi[a]-lo[a])/(2*scale);
    result = std::move(value);
    return true;
}
}
