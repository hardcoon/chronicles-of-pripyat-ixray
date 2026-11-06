#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

// COP_ENGINE_PATCH: COP-ENG-192 - CPU equivalent of pf_sss_puddles.hlsli coverage.
// No camera derivatives: sample the finest resident mip at the contact point.
namespace PFPuddleContact
{
inline bool finite(float value)
{
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7f800000u) != 0x7f800000u;
}
inline float saturate(float value) { return value < 0.f ? 0.f : value > 1.f ? 1.f : value; }
inline float smooth(float low, float high, float value)
{
    const float t = saturate((value - low) / (high - low));
    return t * t * (3.f - 2.f * t);
}
inline float unpackUV(std::int16_t tc, unsigned char fraction)
{
    return (float(tc) + float(fraction) / 255.f) * (32.f / 32768.f);
}
inline float sampleWrap(const unsigned char* red, unsigned width, unsigned height, float u, float v)
{
    if (!red || !width || !height || !finite(u) || !finite(v)) return 1.f;
    const float x = (u - std::floor(u)) * float(width) - .5f;
    const float y = (v - std::floor(v)) * float(height) - .5f;
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const float fx = x - float(ix), fy = y - float(iy);
    const unsigned x0 = unsigned((ix + int(width)) % int(width)), x1 = (x0 + 1) % width;
    const unsigned y0 = unsigned((iy + int(height)) % int(height)), y1 = (y0 + 1) % height;
    const float a = float(red[y0 * width + x0]), b = float(red[y0 * width + x1]);
    const float c = float(red[y1 * width + x0]), d = float(red[y1 * width + x1]);
    return ((a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy) / 255.f;
}
inline float coverage(float mask, float noise, float normalY, float wetness, float size, float border)
{
    const float threshold = 1.f - size * 1.1f;
    float puddle = saturate((noise - threshold) * wetness);
    puddle *= smooth(.90f, .995f, normalY) * (1.f - mask);
    const float edge = .3f - border * .3f;
    return smooth(0.f, edge > .0001f ? edge : .0001f, puddle);
}
}
