#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace masharif {
    class Node;

    /// One flex line. Items are stored as an index range into the owning solve's in-flow
    /// arena slice, so a line never allocates.
    struct FlexLine {
        std::size_t ItemBegin = 0;
        std::size_t ItemEnd = 0;
        int NumberOfAutoMargin = 0;
        float TakenSize = 0;
        float TotalFlexGrow = 0;
        float TotalFlexShrinkScaledFactors = 0;
        float CrossSize = 0;
    };

    /// RAII window over the top of a flat arena: records the base on construction and
    /// truncates back to it on destruction, so nested (recursive) layout runs share one
    /// buffer. The arena may reallocate while a deeper run appends, therefore no
    /// reference, pointer, iterator, or span into an arena may be held across a
    /// recursive layout call — re-index through the slice instead.
    template<typename T>
    class ArenaSlice {
    public:
        explicit ArenaSlice(std::vector<T> &arena) noexcept
            : m_Arena(&arena), m_Base(arena.size()) {
        }

        ~ArenaSlice() { m_Arena->resize(m_Base); }

        ArenaSlice(const ArenaSlice &) = delete;

        ArenaSlice &operator=(const ArenaSlice &) = delete;

        void Append(T value) { m_Arena->push_back(std::move(value)); }

        void Clear() noexcept { m_Arena->resize(m_Base); }

        /// Grow/shrink the slice to exactly `count` elements (value-initialized).
        void Resize(std::size_t count) { m_Arena->resize(m_Base + count); }

        [[nodiscard]] std::size_t Count() const noexcept { return m_Arena->size() - m_Base; }

        [[nodiscard]] bool Empty() const noexcept { return Count() == 0; }

        [[nodiscard]] T &operator[](std::size_t i) noexcept { return (*m_Arena)[m_Base + i]; }

        /// Iterators are valid only at balanced points (no recursive layout call between
        /// obtaining and consuming them).
        [[nodiscard]] auto begin() noexcept { return m_Arena->begin() + static_cast<std::ptrdiff_t>(m_Base); }

        [[nodiscard]] auto end() noexcept { return m_Arena->end(); }

    private:
        std::vector<T> *m_Arena;
        std::size_t m_Base;
    };

    /// Per-solve scratch shared by every strategy run of one layout pass. Created on the
    /// stack by the public entry points and threaded by reference through the recursion;
    /// capacities stay warm for the whole solve, so steady-state layout does not allocate.
    struct LayoutContext {
        /// Warm the scratch arenas so the first solve of a frame does not pay a geometric
        /// reallocation series as items/lines are appended. Capacity-only — never changes
        /// size(), so every ArenaSlice base offset and the re-indexing rule are unaffected
        /// (it can only reduce reallocations, never introduce one).
        LayoutContext() {
            InFlowItems.reserve(64);
            Lines.reserve(8);
            BaseSizes.reserve(64);
            Frozen.reserve(64);
        }

        std::vector<Node *> InFlowItems;
        std::vector<FlexLine> Lines;
        std::vector<float> BaseSizes;
        std::vector<std::uint8_t> Frozen;

        /// Innermost enclosing scroll port on the current StartUpdatingPositions descent and
        /// its offset, threaded (save/restore) down the walk so a sticky descendant pins
        /// against the right port. ForcePin is set while inside a port whose offset changed
        /// this frame, forcing the gated walk past the (unmoved) intermediates to re-pin.
        Node *ScrollPort = nullptr;
        float ScrollPortOffsetX = 0.0f;
        float ScrollPortOffsetY = 0.0f;
        bool ForcePin = false;
    };
}
