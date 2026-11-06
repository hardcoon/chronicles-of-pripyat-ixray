#pragma once
#include <cmath>
#include <ngx/nvsdk_ngx_helpers.h>

// COP_ENGINE_PATCH: COP-ENG-174 - the UI quality and NGX feature must agree.
namespace dlss_contract
{
inline NVSDK_NGX_PerfQuality_Value quality(unsigned preset, float scale)
{
    if (preset == 5)
    {
        if (scale >= .9f) return NVSDK_NGX_PerfQuality_Value_DLAA;
        if (scale >= .7f) return NVSDK_NGX_PerfQuality_Value_MaxQuality;
        if (scale >= .6f) return NVSDK_NGX_PerfQuality_Value_Balanced;
        if (scale >= .5f) return NVSDK_NGX_PerfQuality_Value_MaxPerf;
        return NVSDK_NGX_PerfQuality_Value_UltraPerformance;
    }
    switch (preset)
    {
    case 1: return NVSDK_NGX_PerfQuality_Value_MaxQuality;
    case 2: return NVSDK_NGX_PerfQuality_Value_Balanced;
    case 3: return NVSDK_NGX_PerfQuality_Value_MaxPerf;
    case 4: return NVSDK_NGX_PerfQuality_Value_UltraPerformance;
    default: return NVSDK_NGX_PerfQuality_Value_DLAA;
    }
}
inline const char* hint(NVSDK_NGX_PerfQuality_Value mode)
{
    switch (mode)
    {
    case NVSDK_NGX_PerfQuality_Value_MaxQuality: return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality;
    case NVSDK_NGX_PerfQuality_Value_Balanced: return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced;
    case NVSDK_NGX_PerfQuality_Value_MaxPerf: return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance;
    case NVSDK_NGX_PerfQuality_Value_UltraPerformance: return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance;
    default: return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;
    }
}
inline NVSDK_NGX_DLSS_Create_Params creation(unsigned preset, float scale,
    unsigned width, unsigned height, unsigned targetWidth, unsigned targetHeight)
{
    NVSDK_NGX_DLSS_Create_Params result = {};
    result.Feature.InWidth = width; result.Feature.InHeight = height;
    result.Feature.InTargetWidth = targetWidth; result.Feature.InTargetHeight = targetHeight;
    result.Feature.InPerfQualityValue = quality(preset, scale);
    // The renderer supplies HDR before eye adaptation and no exposure texture.
    result.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
        NVSDK_NGX_DLSS_Feature_Flags_IsHDR | NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
    return result;
}
inline float frame_time(float milliseconds)
{
    return std::isfinite(milliseconds) && milliseconds > 0.f ? milliseconds : 1.f;
}
}
