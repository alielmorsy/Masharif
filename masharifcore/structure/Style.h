#ifndef STYLE_H
#define STYLE_H

#include "Border.h"
#include "Dimension.h"
#include "Edge.h"
#include "Flex.h"
#include <type_traits>

namespace
_NAMESPACE {
    struct MarginEdge : Edge {
    };

    struct PaddingEdge : Edge {
    };

    struct PositionOffsets : Edge {
    };



class Style {
public:
    bool dirty = true;

    template<typename T, std::enable_if_t<
        std::is_same_v<T, Dimensions> ||
        std::is_same_v<T, CSSFlex> ||
        std::is_same_v<T, MarginEdge> ||
        std::is_same_v<T, PaddingEdge> ||
        std::is_same_v<T, Edge> ||
        std::is_same_v<T, BorderProperties>, int> = 0>
    T& modify() {
        dirty = true;
        return getProperty<T>();
    }

    [[nodiscard]] const CSSFlex& flex() const { return m_flexProps; }
    [[nodiscard]] const MarginEdge& margin() const { return m_marginProps; }
    [[nodiscard]] const PaddingEdge& padding() const { return m_paddingProps; }
    [[nodiscard]] const BorderProperties& border() const { return m_borderProps; }
    [[nodiscard]] const Dimensions& dimensions() const { return m_dimensions; }
    [[nodiscard]] const PositionOffsets& offsets() const { return m_offsets; }

private:
    // Use function overloading instead of if constexpr
    template<typename T>
    typename std::enable_if<std::is_same<T, CSSFlex>::value, T&>::type getProperty() {
        return m_flexProps;
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, MarginEdge>::value, T&>::type getProperty() {
        return m_marginProps;
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, PaddingEdge>::value, T&>::type getProperty() {
        return m_paddingProps;
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, BorderProperties>::value, T&>::type getProperty() {
        return m_borderProps;
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, Dimensions>::value, T&>::type getProperty() {
        return m_dimensions;
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, PositionOffsets>::value, T&>::type getProperty() {
        return m_offsets;
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
