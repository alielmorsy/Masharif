#pragma once

#include <cmath>
#include <masharifcore/macros.h>

namespace masharif {
    enum class CSSUnit {
        PX = 0,
        PERCENT,
        AUTO
    };

    struct CSSValue {
        float value = 0.0f;
        CSSUnit unit = CSSUnit::AUTO;

        CSSValue(const float val = NAN) : value(val), unit(std::isnan(val) ? CSSUnit::AUTO : CSSUnit::PX) {
        }

        CSSValue(const float val, CSSUnit u) : value(val), unit(u) {
        }

        /// Resolve to pixels: PX returns the raw value, PERCENT is taken against
        /// `reference`, AUTO resolves to 0.
        [[nodiscard]] constexpr float resolveValue(float reference) const {
            switch (unit) {
                case CSSUnit::PX: return value;
                case CSSUnit::PERCENT: return reference * (value / 100.0f);
                default: return 0.0f;
            }
        }

        [[nodiscard]] bool isUndefined() const {
            return std::isnan(value);
        }

        constexpr bool operator==(const CSSValue &rhs) const {
            return value == rhs.value && unit == rhs.unit;
        }

        constexpr float operator+(const CSSValue &rhs) const { return value + rhs.value; }
        constexpr float operator-(const CSSValue &rhs) const { return value - rhs.value; }
        constexpr float operator*(const CSSValue &rhs) const { return value * rhs.value; }
        constexpr float operator/(const CSSValue &rhs) const { return value / rhs.value; }

        constexpr float operator+(float rhs) const { return value + rhs; }
        constexpr float operator-(float rhs) const { return value - rhs; }
        constexpr float operator*(float rhs) const { return value * rhs; }
        constexpr float operator/(float rhs) const { return value / rhs; }

        friend constexpr float operator+(float lhs, const CSSValue &rhs) { return lhs + rhs.value; }
        friend constexpr float operator-(float lhs, const CSSValue &rhs) { return lhs - rhs.value; }
        friend constexpr float operator*(float lhs, const CSSValue &rhs) { return lhs * rhs.value; }
        friend constexpr float operator/(float lhs, const CSSValue &rhs) { return lhs / rhs.value; }
    };
}
