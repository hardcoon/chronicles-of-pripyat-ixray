// COP_ENGINE_PATCH: COP-ENG-259 - shared, validated Winter snow-pressure tuning.
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace cop_winter_pressure
{
struct Parameter
{
    const char* section;
    const char* key;
    float fallback, minimum, maximum;
};

constexpr std::array<Parameter, 16> parameters{{
    {"actor", "radius_base", 0.46f, 0.05f, 6.0f},
    {"actor", "radius_depth_scale", 0.52f, 0.0f, 4.0f},
    {"actor", "radius_depth_cap", 1.5f, 0.01f, 3.0f},
    {"actor", "depth_fraction", 0.60f, 0.0f, 1.0f},
    {"vehicle", "radius_multiplier", 1.0f, 0.25f, 4.0f},
    {"vehicle", "radius_padding", 0.0f, 0.0f, 4.0f},
    {"vehicle", "depth_max", 0.69f, 0.0f, 3.0f},
    {"vehicle", "depth_fraction", 0.88f, 0.0f, 1.0f},
    {"creature", "radius_base", 1.0f, 0.05f, 6.0f},
    {"creature", "radius_class_step", 0.14f, 0.0f, 1.0f},
    {"creature", "radius_depth_scale", 0.18f, 0.0f, 4.0f},
    {"creature", "radius_depth_cap", 0.9f, 0.01f, 3.0f},
    {"shape", "vertical_start", 0.27f, 0.0f, 2.0f},
    {"shape", "vertical_end", 1.65f, 0.01f, 8.0f},
    {"shape", "radial_power", 2.0f, 1.0f, 8.0f},
    {"shape", "creature_depth_fraction", 0.72f, 0.0f, 1.0f}
}};

using Values = std::array<float, parameters.size()>;

inline float parse(const char* text, const Parameter& parameter)
{
    if (!text)
        return parameter.fallback;
    char* end = nullptr;
    const float value = std::strtof(text, &end);
    if (end == text || !std::isfinite(value))
        return parameter.fallback;
    while (*end && std::isspace(static_cast<unsigned char>(*end)))
        ++end;
    if (*end)
        return parameter.fallback;
    return std::max(parameter.minimum, std::min(parameter.maximum, value));
}

template<typename Reader> Values load(Reader read)
{
    Values values{};
    for (unsigned i = 0; i < parameters.size(); ++i)
        values[i] = parse(read(parameters[i].section, parameters[i].key), parameters[i]);
    values[13] = std::max(values[13], values[12] + 0.01f);
    return values;
}
}
