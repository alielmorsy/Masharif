#ifndef CSSVALUE_H
#define CSSVALUE_H
#include <cmath>
#include <masharifcore/macros.h>
namespace  _NAMESPACE {
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
