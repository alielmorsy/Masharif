#include "LayoutStrategy.h"

#include "FlexLayoutStrategy.h"
#include "NormalFlowStrategy.h"

using namespace masharif;

const LayoutStrategy &LayoutStrategy::For(const OuterDisplay display) noexcept {
    static const FlexLayoutStrategy flex{};
    static const NormalFlowStrategy normalFlow{};
    if (display == OuterDisplay::Block || display == OuterDisplay::InlineBlock)
        return normalFlow;
    return flex;
}
