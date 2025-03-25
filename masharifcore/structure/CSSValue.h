//
// Created by Ali Elmorsy on 3/24/2025.
//

#ifndef CSSVALUE_H
#define CSSVALUE_H
#include <cmath>

namespace masharif {
    enum class CSSUnit {
        PX = 0,
        PERCENT,
        AUTO
    };

    struct CSSValue {
        float value = 0.0f;
        CSSUnit unit = CSSUnit::AUTO;

        explicit CSSValue(const float val = NAN, CSSUnit u = CSSUnit::AUTO) : value(val), unit(u) {
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
    };
}
#endif
