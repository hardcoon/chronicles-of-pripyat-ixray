#pragma once

// COP_ENGINE_PATCH: COP-ENG-261 - independent glyph geometry.
// COP_ENGINE_PATCH: COP-ENG-262 - shared caption fit check.
// COP_ENGINE_PATCH: COP-ENG-263 - caption geometry is independent of XML button widths.
namespace PdaFilterLayout
{
constexpr float LeftSkinCap = 20.f;
constexpr float RightSkinCap = 1.f;
constexpr float TextPadding = 1.5f;

// COP_ENGINE_PATCH: COP-ENG-264 - text is aligned inside the configured insets.
struct CaptionPlacement
{
    float offset;
    float width;
};

inline CaptionPlacement PlaceCaption(float width, float left, float right, float scale, float anchor)
{
    const float insets = (left + right) * scale;
    return {left * scale - insets * anchor, width - insets};
}

struct Segments
{
    float sourceLeft, sourceRight, left, middle, right;
};
inline Segments Split(float width, float height, float sourceHeight, float kx)
{
    const float scale = height / sourceHeight;
    const float left = LeftSkinCap * scale * kx;
    const float right = RightSkinCap * scale * kx;
    return {LeftSkinCap, RightSkinCap, left, width - left - right, right};
}
}
