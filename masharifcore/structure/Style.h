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

#include <type_traits>

class Style {
public:
    bool dirty = true;

    template<typename T, typename std::enable_if<
        std::is_same<T, Dimensions>::value ||
        std::is_same<T, CSSFlex>::value ||
        std::is_same<T, MarginEdge>::value ||
        std::is_same<T, PaddingEdge>::value ||
        std::is_same<T, Edge>::value ||
        std::is_same<T, BorderProperties>::value, int>::type = 0>
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
