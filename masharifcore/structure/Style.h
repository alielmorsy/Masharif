#pragma once

#include "Border.h"
#include "Dimension.h"
#include "Edge.h"
#include "Flex.h"
#include <type_traits>

namespace masharif {
    class Node;

    struct MarginEdge : Edge {
    };

    struct PaddingEdge : Edge {
    };

    struct PositionOffsets : Edge {
    };


    class Style {
    public:
        bool Dirty = true;

        Style() = default;
        Style(const Style &) = delete;
        Style &operator=(const Style &) = delete;
        Style(Style &&) = delete;
        Style &operator=(Style &&) = delete;

        /// Wired by the owning Node at construction so Modify() can propagate dirtiness
        /// up the tree without the caller having to call MarkDirtyToRoot manually.
        void SetOwner(Node *owner) noexcept { m_Owner = owner; }

        template<typename T, std::enable_if_t<
            std::is_same_v<T, Dimensions> ||
            std::is_same_v<T, CSSFlex> ||
            std::is_same_v<T, MarginEdge> ||
            std::is_same_v<T, PaddingEdge> ||
            std::is_same_v<T, Edge> ||
            std::is_same_v<T, BorderProperties>, int> = 0>
        T &Modify() {
            Dirty = true;
            NotifyOwner();
            return GetProperty<T>();
        }

        [[nodiscard]] const CSSFlex &GetFlex() const { return m_FlexProps; }
        [[nodiscard]] const MarginEdge &GetMargin() const { return m_MarginProps; }
        [[nodiscard]] const PaddingEdge &GetPadding() const { return m_PaddingProps; }
        [[nodiscard]] const BorderProperties &GetBorder() const { return m_BorderProps; }
        [[nodiscard]] const Dimensions &GetDimensions() const { return m_Dimensions; }
        [[nodiscard]] const PositionOffsets &GetOffsets() const { return m_Offsets; }

    private:
        void NotifyOwner();

        template<typename T>
        std::enable_if_t<std::is_same_v<T, CSSFlex>, T &> GetProperty() {
            return m_FlexProps;
        }

        template<typename T>
        std::enable_if_t<std::is_same_v<T, MarginEdge>, T &> GetProperty() {
            return m_MarginProps;
        }

        template<typename T>
        std::enable_if_t<std::is_same_v<T, PaddingEdge>, T &> GetProperty() {
            return m_PaddingProps;
        }

        template<typename T>
        std::enable_if_t<std::is_same_v<T, BorderProperties>, T &> GetProperty() {
            return m_BorderProps;
        }

        template<typename T>
        std::enable_if_t<std::is_same_v<T, Dimensions>, T &> GetProperty() {
            return m_Dimensions;
        }

        template<typename T>
        std::enable_if_t<std::is_same_v<T, PositionOffsets>, T &> GetProperty() {
            return m_Offsets;
        }

        Node *m_Owner = nullptr;

        // Property storage
        CSSFlex m_FlexProps;
        MarginEdge m_MarginProps;
        PaddingEdge m_PaddingProps;
        PositionOffsets m_Offsets;
        BorderProperties m_BorderProps;
        Dimensions m_Dimensions;
    };
}
