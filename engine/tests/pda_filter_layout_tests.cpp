#include "../src/xrGame/ui/PdaFilterLayout.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>

static bool close(float a, float b) { return std::fabs(a - b) < 0.002f; }

int main()
{
    using namespace PdaFilterLayout;
    unsigned cases = 0;
    const float resolutions[][2] = {{800,600},{1024,768},{1280,960},{1280,1024},
        {1920,1080},{2560,1440},{2560,1080},{3440,1440}};
    for (const auto& resolution : resolutions)
    {
        const float scaleX = resolution[0] / 1024.f;
        const float scaleY = resolution[1] / 768.f;
        const float kx = scaleY / scaleX;
        for (float width : {50.f,70.f,94.f,112.f,143.f,150.f,220.f})
            for (float height : {19.f,22.f,28.f})
            {
                const auto parts = Split(width,height,19.f,kx);
                assert(parts.middle > 0.f);
                assert(close(parts.left + parts.middle + parts.right,width));
                // Every source texel of the round indicator gets equal physical X/Y scale.
                assert(close(parts.left / parts.sourceLeft * scaleX,height / 19.f * scaleY));
                assert(close(parts.right / parts.sourceRight * scaleX,height / 19.f * scaleY));
                ++cases;
            }
        const float width = 115.f;
        const auto left = PlaceCaption(width, 17.f, 3.f, kx, 0.f);
        const auto center = PlaceCaption(width, 17.f, 3.f, kx, 0.5f);
        const auto right = PlaceCaption(width, 17.f, 3.f, kx, 1.f);
        assert(close(left.offset, 17.f * kx));
        assert(close(center.offset + width / 2.f, (left.offset + width + right.offset) / 2.f));
        assert(close(right.offset, -3.f * kx));
        assert(close(left.offset + left.width, width + right.offset));
        assert(close(left.width, center.width) && close(left.width, right.width));
        ++cases;
    }
    std::printf("PASS: %u layout/aspect cases; caption bounds and round icons\n", cases);
}
