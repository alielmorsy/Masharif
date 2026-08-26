#include <gtest/gtest.h>
#include <masharifcore/Masharif.h>

using namespace masharif;

TEST(ComputedMarginTests, auto_margin) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    root->GetStyle().Modify<Dimensions>().Width = 50.0f;
    root->GetStyle().Modify<Dimensions>().Height = 50.0f;

    auto root_child0 = std::make_shared<Node>();
    // Set margin-left: auto using NAN
    root_child0->GetStyle().Modify<MarginEdge>().Left = {NAN, CSSUnit::Auto};
    root_child0->GetStyle().Modify<Dimensions>().Width = 25.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 25.0f;
    root->AddChild(root_child0);

    root->Calculate(NAN, NAN);

    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(50.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(50.0f, root->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(25.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(25.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(25.0f, root_child0->GetLayout().ComputedHeight);
}

TEST(ComputedMarginTests, computed_layout_margin_percentage) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto child = std::make_shared<Node>();
    child->GetStyle().Modify<Dimensions>().Width = 50.0f;
    child->GetStyle().Modify<Dimensions>().Height = 50.0f;
    // simulating YGNodeStyleSetMarginPercent(root, YGEdgeStart, 10);
    // Since we don't have Start, we use Left for LTR test.
    root->AddChild(child);
    
    // Set margin-left: 10%
    child->GetStyle().Modify<MarginEdge>().Left = {10.0f, CSSUnit::Percent};

    root->Calculate(100.0f, 100.0f);

    // 10% of 100 = 10.
    ASSERT_FLOAT_EQ(10.0f, child->GetLayout().ComputedX);
}

TEST(ComputedMarginTests, margin_side_overrides_horizontal) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto child = std::make_shared<Node>();
    child->GetStyle().Modify<Dimensions>().Width = 10.0f;
    child->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(child);

    child->GetStyle().Modify<MarginEdge>().Left = 10.0f;
    child->GetStyle().Modify<MarginEdge>().Right = 10.0f;
    
    // Override Left
    child->GetStyle().Modify<MarginEdge>().Left = 20.0f;

    root->Calculate(100.0f, 100.0f);

    ASSERT_FLOAT_EQ(20.0f, child->GetLayout().ComputedX);
}

/// CSS Flexbox 8.1: a main-axis `auto` margin takes free space only on the side it is declared
/// on. `margin-right: auto` therefore leaves the item flush at the line's start and pushes its
/// SIBLINGS away; the engine used to write the share into both sides, moving the item itself
/// forward by the whole of the free space and running the line past the container's end edge.
TEST(ComputedMarginTests, auto_end_margin_pushes_siblings_not_the_item_itself) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto first = std::make_shared<Node>();
    first->GetStyle().Modify<Dimensions>().Width = 70.0f;
    first->GetStyle().Modify<Dimensions>().Height = 40.0f;
    first->GetStyle().Modify<MarginEdge>().Right = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(first);

    auto second = std::make_shared<Node>();
    second->GetStyle().Modify<Dimensions>().Width = 70.0f;
    second->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(second);

    root->Calculate(300.0f, 100.0f);

    // 300 - 140 = 160 of free space, all of it between the two items.
    ASSERT_FLOAT_EQ(0.0f, first->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(230.0f, second->GetLayout().ComputedX);
}

/// The mirror image: an `auto` START margin on the trailing item pins that item to the line's end
/// edge and leaves the item before it where it was.
TEST(ComputedMarginTests, auto_start_margin_pins_the_item_to_the_line_end) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto first = std::make_shared<Node>();
    first->GetStyle().Modify<Dimensions>().Width = 70.0f;
    first->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(first);

    auto second = std::make_shared<Node>();
    second->GetStyle().Modify<Dimensions>().Width = 70.0f;
    second->GetStyle().Modify<Dimensions>().Height = 40.0f;
    second->GetStyle().Modify<MarginEdge>().Left = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(second);

    root->Calculate(300.0f, 100.0f);

    ASSERT_FLOAT_EQ(0.0f, first->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(230.0f, second->GetLayout().ComputedX);
}

/// Both margins auto still splits the free space in half per side -- the free space is divided by
/// the number of auto MARGINS on the line, not the number of items carrying one, so the symmetric
/// centring idiom is unaffected by the per-side fix.
TEST(ComputedMarginTests, both_auto_main_margins_centre_the_item) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto child = std::make_shared<Node>();
    child->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child->GetStyle().Modify<Dimensions>().Height = 40.0f;
    child->GetStyle().Modify<MarginEdge>().Left = CSSValue(0.0f, CSSUnit::Auto);
    child->GetStyle().Modify<MarginEdge>().Right = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(child);

    root->Calculate(300.0f, 100.0f);

    ASSERT_FLOAT_EQ(100.0f, child->GetLayout().ComputedX);
}

/// Auto margins only ever absorb POSITIVE free space. An overflowing line has none to give, so
/// they resolve to 0 and the items pack from the start edge -- rather than each auto margin taking
/// a share of the overflow and dragging the line backwards off its own container.
TEST(ComputedMarginTests, auto_main_margins_are_zero_on_an_overflowing_line) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto first = std::make_shared<Node>();
    first->GetStyle().Modify<Dimensions>().Width = 70.0f;
    first->GetStyle().Modify<Dimensions>().Height = 40.0f;
    first->GetStyle().Modify<CSSFlex>().FlexShrink = 0.0f;
    first->GetStyle().Modify<MarginEdge>().Right = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(first);

    auto second = std::make_shared<Node>();
    second->GetStyle().Modify<Dimensions>().Width = 70.0f;
    second->GetStyle().Modify<Dimensions>().Height = 40.0f;
    second->GetStyle().Modify<CSSFlex>().FlexShrink = 0.0f;
    root->AddChild(second);

    root->Calculate(100.0f, 100.0f);

    ASSERT_FLOAT_EQ(0.0f, first->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(70.0f, second->GetLayout().ComputedX);
}

/// CSS Flexbox runs 9.7 (resolve flexible lengths) BEFORE 8.1 (auto margins claim what is left).
/// A flexible sibling therefore grows into the free space first and the auto margin gets whatever
/// survives -- nothing, here. The solver used to bail out of flex resolution entirely on any line
/// carrying an auto margin, which froze every item on it at its flex base size.
TEST(ComputedMarginTests, flexible_lengths_resolve_before_auto_margins_claim_space) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto grower = std::make_shared<Node>();
    grower->GetStyle().Modify<Dimensions>().Width = 30.0f;
    grower->GetStyle().Modify<Dimensions>().Height = 40.0f;
    grower->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root->AddChild(grower);

    auto pinned = std::make_shared<Node>();
    pinned->GetStyle().Modify<Dimensions>().Width = 70.0f;
    pinned->GetStyle().Modify<Dimensions>().Height = 40.0f;
    pinned->GetStyle().Modify<MarginEdge>().Left = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(pinned);

    root->Calculate(300.0f, 100.0f);

    // 300 - 30 - 70 = 200 of free space, all of it to the grow item; the auto margin gets 0.
    ASSERT_FLOAT_EQ(230.0f, grower->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(0.0f, grower->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(230.0f, pinned->GetLayout().ComputedX);
}

/// CSS Flexbox 8.1: auto margins absorb ALL the positive free space before justify-content is
/// consulted, so a line carrying one leaves justify-content nothing to distribute. Applying both
/// pushed the item past the container's end edge by the justify offset.
TEST(ComputedMarginTests, auto_margin_leaves_justify_content_nothing_to_distribute) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexCenter;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto child = std::make_shared<Node>();
    child->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child->GetStyle().Modify<Dimensions>().Height = 40.0f;
    child->GetStyle().Modify<MarginEdge>().Left = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(child);

    root->Calculate(300.0f, 100.0f);

    // margin-left takes the whole 200; centring must not add another 100 on top of it.
    ASSERT_FLOAT_EQ(200.0f, child->GetLayout().ComputedX);
}

/// The gap and the items' own non-auto margins are part of what the line consumes, so neither
/// counts as free space for an auto margin to claim.
TEST(ComputedMarginTests, auto_margin_share_excludes_gaps_and_non_auto_margins) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Gaps.Column = 20.0f;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto first = std::make_shared<Node>();
    first->GetStyle().Modify<Dimensions>().Width = 70.0f;
    first->GetStyle().Modify<Dimensions>().Height = 40.0f;
    first->GetStyle().Modify<MarginEdge>().Right = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(first);

    auto second = std::make_shared<Node>();
    second->GetStyle().Modify<Dimensions>().Width = 70.0f;
    second->GetStyle().Modify<Dimensions>().Height = 40.0f;
    second->GetStyle().Modify<MarginEdge>().Left = 10.0f; // non-auto: consumed, not free
    root->AddChild(second);

    root->Calculate(300.0f, 100.0f);

    // 300 - 70 - 70 - 20 (gap) - 10 (margin-left) = 130 to the one auto margin.
    ASSERT_FLOAT_EQ(0.0f, first->GetLayout().ComputedX);
    // 70 + 130 (auto) + 20 (gap) + 10 (its own margin-left) = 230.
    ASSERT_FLOAT_EQ(230.0f, second->GetLayout().ComputedX);
}
