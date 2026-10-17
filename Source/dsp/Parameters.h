#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>

namespace gba {
enum class Param : std::size_t {
#define GBA_PARAM(symbol, ...) symbol,
#include "ParameterList.inc"
#undef GBA_PARAM
    Count
};
constexpr auto parameterCount = static_cast<std::size_t>(Param::Count);
struct ParamSpec {
    std::string_view id, name;
    float min, max, initial, step, skew;
    std::string_view choices;
};
inline constexpr std::array<ParamSpec, parameterCount> specs {{
#define GBA_PARAM(symbol, id, name, lo, hi, def, step, skew, choices) {id, name, lo, hi, def, step, skew, choices},
#include "ParameterList.inc"
#undef GBA_PARAM
}};
constexpr std::size_t index(Param p) { return static_cast<std::size_t>(p); }
struct Settings {
    std::array<float, parameterCount> values {};
    Settings() { for (std::size_t i=0; i<values.size(); ++i) values[i] = specs[i].initial; }
    float operator[](Param p) const noexcept { return values[index(p)]; }
    void set(Param p, float v) noexcept {
        const auto& s = specs[index(p)];
        if (!std::isfinite(v)) v = s.initial;
        v = std::clamp(v, s.min, s.max);
        if (s.step > 0) v = s.min + std::round((v-s.min)/s.step)*s.step;
        values[index(p)] = std::clamp(v, s.min, s.max);
    }
};
inline constexpr std::array<Param, 8> macroTargets {Param::Cutoff, Param::Resonance,
    Param::Attack, Param::Reverb, Param::FmIndex, Param::Vowel, Param::Delay, Param::Drive};
}
