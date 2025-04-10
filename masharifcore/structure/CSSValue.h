#ifndef CSSVALUE_H
#define CSSVALUE_H
#include <cmath>

namespace
_NAMESPACE {
    enum class CSSUnit {
        PX = 0,
        PERCENT,
        AUTO
    };

    struct CSSValue {
        float value = 0.0f;
        CSSUnit unit = CSSUnit::AUTO;

        explicit CSSValue(const float val = 0, CSSUnit u = CSSUnit::AUTO) : value(val), unit(u) {
        }

        [[nodiscard]] float resolveValue(float reference) const {
            switch (unit) {
                case CSSUnit::PX: return value;
                case CSSUnit::PERCENT: return reference * (value / 100.0f);
                default: return 0.0f;
            }
        }

        [[nodiscard]] bool isUndefined() const {
            return std::isnan(value);
        }

        inline bool operator==( const CSSValue &rhs) const {
            return value == rhs.value && unit == rhs.unit;
        }
    };
}
#endif
