#pragma once

#include <cmath>
#include <masharifcore/macros.h>

namespace masharif {
    enum class CSSUnit {
        Px = 0,
        Percent,
        Auto
    };

    struct CSSValue {
        float Value = 0.0f;
        CSSUnit Unit = CSSUnit::Auto;

        CSSValue(const float val = NAN) : Value(val), Unit(std::isnan(val) ? CSSUnit::Auto : CSSUnit::Px) {
        }

        CSSValue(const float val, CSSUnit u) : Value(val), Unit(u) {
        }

        /// Resolve to pixels: Px returns the raw value, Percent is taken against
        /// `reference`, Auto resolves to 0.
        [[nodiscard]] constexpr float ResolveValue(float reference) const {
            switch (Unit) {
                case CSSUnit::Px: return Value;
                case CSSUnit::Percent: return reference * (Value / 100.0f);
                default: return 0.0f;
            }
        }

        [[nodiscard]] bool IsUndefined() const {
            return std::isnan(Value);
        }

        constexpr bool operator==(const CSSValue &rhs) const {
            return Value == rhs.Value && Unit == rhs.Unit;
        }

        constexpr float operator+(const CSSValue &rhs) const { return Value + rhs.Value; }
        constexpr float operator-(const CSSValue &rhs) const { return Value - rhs.Value; }
        constexpr float operator*(const CSSValue &rhs) const { return Value * rhs.Value; }
        constexpr float operator/(const CSSValue &rhs) const { return Value / rhs.Value; }

        constexpr float operator+(float rhs) const { return Value + rhs; }
        constexpr float operator-(float rhs) const { return Value - rhs; }
        constexpr float operator*(float rhs) const { return Value * rhs; }
        constexpr float operator/(float rhs) const { return Value / rhs; }

        friend constexpr float operator+(float lhs, const CSSValue &rhs) { return lhs + rhs.Value; }
        friend constexpr float operator-(float lhs, const CSSValue &rhs) { return lhs - rhs.Value; }
        friend constexpr float operator*(float lhs, const CSSValue &rhs) { return lhs * rhs.Value; }
        friend constexpr float operator/(float lhs, const CSSValue &rhs) { return lhs / rhs.Value; }
    };
}
