#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <masharifcore/structure/Style.h>


#include "Layout.h"


namespace masharif
{
    class Node;
    struct LayoutContext;
    using SharedNode = std::shared_ptr<Node>;

    class Node
    {
    public:
        explicit Node(const OuterDisplay display = OuterDisplay::Block)
        {
            m_Style.SetOwner(this);
            SetDisplay(display);
        }

        [[nodiscard]] Layout& GetLayout() { return m_Layout; }

        [[nodiscard]] Style& GetStyle() { return m_Style; }

        /// Solve this subtree against the given available space, then derive absolute
        /// positions and clear dirty flags: the per-frame entry point.
        void Calculate(float availableWidth, float availableHeight);

        /// Standalone solve at the given available space (builds its own LayoutContext).
        /// Unlike Calculate it neither clears dirty flags nor derives absolute positions.
        void LayoutImpl(float availableWidth, float availableHeight, bool ignoreMinMax = false);

        [[nodiscard]] Node* Parent() const { return m_Parent; }

        [[nodiscard]] const std::vector<SharedNode>& Children() const noexcept { return m_Children; }

        /// Replace the whole child list, reparenting every new child. Previous children are
        /// dropped without being detached (reconciliation semantics: the caller discards
        /// stale handles).
        void SetChildren(std::vector<SharedNode> children)
        {
            for (auto& child : children) child->SetParent(this);
            m_Children = std::move(children);
            MarkDirtyToRoot();
        }

        void ClearChildren()
        {
            m_Children.clear();
            MarkDirtyToRoot();
        }

        void AddChild(const SharedNode& child)
        {
            m_Children.push_back(child);
            child->SetParent(this);
            MarkDirtyToRoot();
        }

        void RemoveChild(SharedNode& child);

        [[nodiscard]] SharedNode FirstChild() const { return m_Children[0]; }

        [[nodiscard]] SharedNode LastChild() const { return m_Children.back(); }

        /// Pure style write: the layout algorithm is selected from the display type at solve
        /// time (LayoutStrategy::For), so switching display allocates nothing.
        void SetDisplay(OuterDisplay display)
        {
            GetStyle().Modify<Dimensions>().Display = display;
        }

        /// Mark this node dirty and flag every ancestor up to the root. Style::Modify calls
        /// this automatically; only mutations done behind the Style API (e.g. a direct
        /// GetStyle().Dirty write) still need it explicitly.
        void MarkDirtyToRoot();

    private:
        friend class FlexLayoutStrategy;
        friend class NormalFlowStrategy;

        void LayoutImpl(LayoutContext& ctx, float availableWidth, float availableHeight,
                        bool ignoreMinMax = false);

        /// Re-lay-out this subtree at an already-resolved definite border-box size (set by the
        /// flex parent's main-axis resolution + the cross-axis stretch). Needed because the
        /// flex-basis phase measures AUTO items against NaN and collapses them to 0.
        void LayoutContentsWithDefiniteSize(LayoutContext& ctx, float borderBoxWidth, float borderBoxHeight);

        void StartUpdatingPositions(LayoutContext& ctx);

        void PositionOutOfFlowChildren(LayoutContext& ctx);

        void ComputeDimensions(LayoutContext& ctx, float availableWidth, float availableHeight,
                               bool ignoreMinMax = false);

        void PositionOutOfFlowChild(Node* ancestor, float refWidth, float refHeight);

        void HandleStickyPosition(float refWidth, float refHeight);

        void SetParent(Node* parent)
        {
            if (m_Parent != parent) ResetFrameStamps();
            m_Parent = parent;
        }

        /// True only while LayoutContentsWithDefiniteSize drives the strategy: the parent has
        /// resolved this node's border box, so the flex strategy must NOT shrink-to-fit an
        /// AUTO main axis (that would re-collapse a cross-stretched grow container).
        [[nodiscard]] bool MainSizeIsDefinite() const { return m_mainSizeDefinite; }

        /// True only while LayoutContentsWithDefiniteSize drives the strategy: the parent fixed
        /// this node's border box on BOTH axes, so a single flex line's cross size is the
        /// container's content box (an item taller than it overflows, it does not enlarge the
        /// line). An AUTO cross axis keeps shrink-wrapping to the tallest item.
        [[nodiscard]] bool CrossSizeIsDefinite() const { return m_crossSizeDefinite; }

        /// One completed LayoutImpl solve of the current frame: inputs -> resulting content
        /// size. Generation == 0 marks an empty entry. See docs/layout-caching.md.
        struct MeasureCacheEntry
        {
            std::uint64_t Generation = 0;
            float AvailW = NAN, AvailH = NAN;
            bool IgnoreMinMax = false;
            float ResultW = NAN, ResultH = NAN;
        };

        static constexpr std::size_t MeasureCacheSize = 4;

        /// Bump the tree-wide frame counter (owned by the root) and return it. Entry points
        /// stamp themselves with it; descendants pull it lazily at solve entry.
        std::uint64_t BumpTreeGeneration()
        {
            Node* root = this;
            while (root->m_Parent) root = root->m_Parent;
            return ++root->m_generation;
        }

        /// Adopt the parent's generation when it is newer; first statement of every solve.
        void PullGeneration()
        {
            if (m_Parent && m_Parent->m_generation > m_generation)
                m_generation = m_Parent->m_generation;
        }

        void RecordMeasure(float availW, float availH, bool ignoreMinMax,
                           float resultW, float resultH);

        [[nodiscard]] const MeasureCacheEntry* FindMeasure(float availW, float availH,
                                                           bool ignoreMinMax) const;

        /// Frame stamps are only comparable within one tree; clear them when this node is
        /// re-parented so entries from another tree's counter can never match.
        void ResetFrameStamps()
        {
            m_generation = 0;
            m_defGeneration = 0;
            m_measureCache = {};
            m_measureCacheNext = 0;
        }

        Node* m_Parent = nullptr;
        Style m_Style{};
        Layout m_Layout{};

        std::vector<SharedNode> m_Children;

        /// Out-of-flow (absolute/fixed/sticky) children diverted by the last strategy run;
        /// consumed (laid out + positioned) by the positions walk, then cleared.
        std::vector<SharedNode> m_OutOfFlowChildren;

        /// Set by MarkDirtyToRoot on every ancestor of a changed node. A node with neither
        /// m_Style.Dirty nor m_DescendantDirty (and unchanged space) reuses its cached layout.
        bool m_descendantDirty = false;

        /// See MainSizeIsDefinite().
        bool m_mainSizeDefinite = false;

        /// See CrossSizeIsDefinite().
        bool m_crossSizeDefinite = false;

        /// True while the descendants reflect an impl-path (available-space) strategy run
        /// rather than the last definite-size distribution; forces the next definite pass to
        /// re-run even when its size memo matches. Subsumes the old shrink-wrap special case.
        bool m_strategyRanSinceDefinite = false;

        /// Set whenever a strategy runs for this node (its children were repositioned);
        /// consumed by the gated StartUpdatingPositions walk.
        bool m_positionsDirty = false;

        /// Tree-frame counter: the root owns the running value (bumped per Calculate /
        /// standalone LayoutImpl); every other node carries the stamp it last solved under.
        std::uint64_t m_generation = 0;

        /// Generation of the last definite-size strategy run (pairs with m_LastDefW/H).
        std::uint64_t m_defGeneration = 0;

        std::array<MeasureCacheEntry, MeasureCacheSize> m_measureCache{};
        std::uint8_t m_measureCacheNext = 0;

        /// Memo of the space each pass last ran against, so a clean subtree is re-solved only
        /// when that space changes. NAN means "never laid out" and forces the first solve.
        float m_lastAvailW = NAN, m_lastAvailH = NAN; ///< LayoutImpl available space
        float m_lastDefW = NAN, m_lastDefH = NAN; ///< LayoutContentsWithDefiniteSize size

        /// Content-box size from the last full LayoutImpl run (before any parent flex
        /// grow/shrink). Restored on the reuse early-out so a clean child reports its content
        /// size for flex-basis derivation rather than a transient grown/collapsed value.
        float m_implW = NAN, m_implH = NAN;
    };
}
