#ifndef STYLE_H
#define STYLE_H
#include <bitset>
#include <functional>
#include <optional>

#include "Border.h"
#include "BoxInfo.h"
#include "Dimension.h"
#include "Edge.h"
#include "Flex.h"

namespace
_NAMESPACE {
    struct MarginEdge : Edge {
    };

    struct PaddingEdge : Edge {
    };

    struct PositionOffsets : Edge {
    };

    template<typename T>
    concept ModifiableProperty = std::is_same_v<T, Dimensions> ||
                                 std::is_same_v<T, CSSFlex> ||
                                 std::is_same_v<T, MarginEdge> ||
                                 std::is_same_v<T, PaddingEdge> ||
                                 std::is_same_v<T, Edge> ||
                                 std::is_same_v<T, BorderProperties>;

    class Style {
    public:
        bool dirty = true;

        template<ModifiableProperty T>
        T &modify() {
            dirty = true;
            return getProperty<T>();
        }



        [[nodiscard]] const CSSFlex &flex() const { return m_flexProps; }
        [[nodiscard]] const MarginEdge &margin() const { return m_marginProps; }
        [[nodiscard]] const PaddingEdge &padding() const { return m_paddingProps; }
        [[nodiscard]] const BorderProperties &border() const { return m_borderProps; }
        [[nodiscard]] const Dimensions &dimensions() const { return m_dimensions; }
        [[nodiscard]] const PositionOffsets &offsets() const { return m_offsets; }

    private:
        template<ModifiableProperty T>
        T &getProperty() {
            if constexpr (std::is_same_v<T, CSSFlex>) {
                return m_flexProps;
            } else if constexpr (std::is_same_v<T, MarginEdge>) {
                return m_marginProps;
            } else if constexpr (std::is_same_v<T, PaddingEdge>) {
                return m_paddingProps;
            } else if constexpr (std::is_same_v<T, BorderProperties>) {
                return m_borderProps;
            } else if constexpr (std::is_same_v<T, Dimensions>) {
                return m_dimensions;
            } else if constexpr (std::is_same_v<T, PositionOffsets>) {
                return m_offsets;
            }
        }

        // Property storage
        CSSFlex m_flexProps;
        MarginEdge m_marginProps;
        PaddingEdge m_paddingProps;
        PositionOffsets m_offsets;
        BorderProperties m_borderProps;
        Dimensions m_dimensions;
    };
};


#endif
