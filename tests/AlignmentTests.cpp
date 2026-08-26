#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(AlignmentTests, justify_content_row_flex_start) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root->AddChild(root_child1);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(10.0f, root_child1->GetLayout().ComputedX);
}

TEST(AlignmentTests, justify_content_row_flex_end) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexEnd;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root->AddChild(root_child1);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(80.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(90.0f, root_child1->GetLayout().ComputedX);
}

TEST(AlignmentTests, justify_content_row_center) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexCenter;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root->AddChild(root_child1);

    root->Calculate(100, 100);

    // (100 - 20) / 2 = 40
    ASSERT_FLOAT_EQ(40.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(50.0f, root_child1->GetLayout().ComputedX);
}

TEST(AlignmentTests, align_items_stretch) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::Stretch;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child0);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(100.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedHeight);
}

TEST(AlignmentTests, align_items_center) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child0);

    root->Calculate(100, 100);

    // (100 - 10) / 2 = 45
    ASSERT_FLOAT_EQ(45.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedY);
}

TEST(AlignmentTests, align_self_flex_end) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root_child0->GetStyle().Modify<CSSFlex>().AlignSelf = AlignItems::FlexEnd;
    root->AddChild(root_child0);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(90.0f, root_child0->GetLayout().ComputedX);
}

// gh#4 -- An AUTO cross size is CONTENT-based (Flexbox 9.4: the item's hypothetical cross size);
// `align-self: stretch` is what expands it to the line afterwards. Filling the line unconditionally
// sizes the item wrong AND leaves every other align-items value with nothing to move.
TEST(AlignmentTests, auto_cross_size_item_is_content_sized_and_centers) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 20.0f;

    // Nested row container with no height of its own: its cross size is its content's.
    auto legend = std::make_shared<Node>();
    legend->SetDisplay(OuterDisplay::Flex);
    legend->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    legend->GetStyle().Modify<CSSFlex>().FlexShrink = 0.0f;
    root->AddChild(legend);

    auto bar = std::make_shared<Node>();
    bar->SetDisplay(OuterDisplay::Flex);
    bar->GetStyle().Modify<Dimensions>().Width = 40.0f;
    bar->GetStyle().Modify<Dimensions>().Height = 10.0f;
    legend->AddChild(bar);

    root->Calculate(200, 20);

    ASSERT_FLOAT_EQ(10.0f, legend->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(5.0f, legend->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(5.0f, bar->GetLayout().ComputedY);
}

// The same AUTO cross size under the DEFAULT align-items: stretch still fills the line -- the
// content-sizing above must not cost the stretch case its expansion.
TEST(AlignmentTests, auto_cross_size_item_still_stretches_to_the_line) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 20.0f;

    auto legend = std::make_shared<Node>();
    legend->SetDisplay(OuterDisplay::Flex);
    legend->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    legend->GetStyle().Modify<CSSFlex>().FlexShrink = 0.0f;
    root->AddChild(legend);

    auto bar = std::make_shared<Node>();
    bar->SetDisplay(OuterDisplay::Flex);
    bar->GetStyle().Modify<Dimensions>().Width = 40.0f;
    bar->GetStyle().Modify<Dimensions>().Height = 10.0f;
    legend->AddChild(bar);

    root->Calculate(200, 20);

    ASSERT_FLOAT_EQ(20.0f, legend->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(0.0f, legend->GetLayout().ComputedY);
}

// The column mirror of the two above: a column container's cross axis is its WIDTH, so an
// AUTO-width item is content-sized there and align-items:center has something to centre.
TEST(AlignmentTests, auto_cross_width_item_is_content_sized_and_centers) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto strip = std::make_shared<Node>();
    strip->SetDisplay(OuterDisplay::Flex);
    strip->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->AddChild(strip);

    auto bar = std::make_shared<Node>();
    bar->SetDisplay(OuterDisplay::Flex);
    bar->GetStyle().Modify<Dimensions>().Width = 40.0f;
    bar->GetStyle().Modify<Dimensions>().Height = 10.0f;
    strip->AddChild(bar);

    root->Calculate(200, 100);

    ASSERT_FLOAT_EQ(40.0f, strip->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(80.0f, strip->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(80.0f, bar->GetLayout().ComputedX);
}

// ...and under the default align-items: stretch it still fills the line's width. This is the case
// the AUTO-width content sizing must not cost anything: a BLOCK-level flex container in normal flow
// fills its containing block outright (gh#3), which is why the content sizing is gated on the box
// actually being a flex item.
TEST(AlignmentTests, auto_cross_width_item_still_stretches_to_the_line) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto strip = std::make_shared<Node>();
    strip->SetDisplay(OuterDisplay::Flex);
    strip->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->AddChild(strip);

    auto bar = std::make_shared<Node>();
    bar->SetDisplay(OuterDisplay::Flex);
    bar->GetStyle().Modify<Dimensions>().Width = 40.0f;
    bar->GetStyle().Modify<Dimensions>().Height = 10.0f;
    strip->AddChild(bar);

    root->Calculate(200, 100);

    ASSERT_FLOAT_EQ(200.0f, strip->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(0.0f, strip->GetLayout().ComputedX);
}

// The block-level counterpart of the guard: a `display: flex` box in NORMAL FLOW keeps filling its
// containing block on the inline axis whatever its own direction, and content-sizes its AUTO height.
TEST(AlignmentTests, normal_flow_flex_column_fills_width_and_shrink_wraps_height) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Block);
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto column = std::make_shared<Node>();
    column->SetDisplay(OuterDisplay::Flex);
    column->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->AddChild(column);

    auto bar = std::make_shared<Node>();
    bar->SetDisplay(OuterDisplay::Flex);
    bar->GetStyle().Modify<Dimensions>().Width = 40.0f;
    bar->GetStyle().Modify<Dimensions>().Height = 10.0f;
    column->AddChild(bar);

    root->Calculate(200, 100);

    ASSERT_FLOAT_EQ(200.0f, column->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(10.0f, column->GetLayout().ComputedHeight);
}

// ---------------------------------------------------------------- align-items: baseline
//
// This engine has no text and no baseline callback, so every box falls into the CSS rule for a
// box with no baseline of its own: the baseline is SYNTHESISED from the bottom margin edge. That
// makes baseline alignment fully determinate here rather than unimplementable -- a row of boxes
// of different heights lines its bottoms up, which is exactly what a browser does with the same
// (textless) markup. See FlexLayoutStrategy::Solver::AlignLinesOnCrossAxis.

TEST(AlignmentTests, align_items_baseline_synthesises_from_the_bottom_edge) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::Baseline;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    const float heights[] = {40.0f, 28.0f, 34.0f};
    std::vector<SharedNode> kids;
    for (const float h: heights) {
        auto child = std::make_shared<Node>();
        child->GetStyle().Modify<Dimensions>().Width = 80.0f;
        child->GetStyle().Modify<Dimensions>().Height = h;
        root->AddChild(child);
        kids.push_back(child);
    }

    root->Calculate(300, 100);

    // Tallest item owns the line's baseline; the others drop by the difference so all three
    // bottoms land on 40.
    ASSERT_FLOAT_EQ(0.0f, kids[0]->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(12.0f, kids[1]->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(6.0f, kids[2]->GetLayout().ComputedY);
}

TEST(AlignmentTests, align_items_baseline_counts_the_leading_cross_margin) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::Baseline;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    // Ascent is margin-top + baseline, so this one's ascent (10 + 30) ties the plain 40 below and
    // its margin box still starts at the line's top edge.
    auto margined = std::make_shared<Node>();
    margined->GetStyle().Modify<Dimensions>().Width = 80.0f;
    margined->GetStyle().Modify<Dimensions>().Height = 30.0f;
    margined->GetStyle().Modify<MarginEdge>().Top = 10.0f;
    root->AddChild(margined);

    auto plain = std::make_shared<Node>();
    plain->GetStyle().Modify<Dimensions>().Width = 80.0f;
    plain->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(plain);

    root->Calculate(300, 100);

    ASSERT_FLOAT_EQ(10.0f, margined->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(0.0f, plain->GetLayout().ComputedY);
}

TEST(AlignmentTests, align_items_baseline_takes_a_container_item_from_its_first_child) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::Baseline;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    // 60 tall, but its baseline comes from the first flex item inside it -- 20, not 60.
    auto nested = std::make_shared<Node>();
    nested->SetDisplay(OuterDisplay::Flex);
    nested->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    nested->GetStyle().Modify<Dimensions>().Width = 80.0f;
    nested->GetStyle().Modify<Dimensions>().Height = 60.0f;
    root->AddChild(nested);

    auto inner = std::make_shared<Node>();
    inner->GetStyle().Modify<Dimensions>().Width = 80.0f;
    inner->GetStyle().Modify<Dimensions>().Height = 20.0f;
    nested->AddChild(inner);

    auto plain = std::make_shared<Node>();
    plain->GetStyle().Modify<Dimensions>().Width = 80.0f;
    plain->GetStyle().Modify<Dimensions>().Height = 30.0f;
    root->AddChild(plain);

    root->Calculate(300, 100);

    // Line ascent is max(20, 30) = 30, so the nested container drops by 10 and the plain box stays.
    ASSERT_FLOAT_EQ(10.0f, nested->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(0.0f, plain->GetLayout().ComputedY);
}

TEST(AlignmentTests, align_items_baseline_in_a_column_container_falls_back_to_flex_start) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::Baseline;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto child = std::make_shared<Node>();
    child->GetStyle().Modify<Dimensions>().Width = 30.0f;
    child->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(child);

    root->Calculate(100, 100);

    // A column container's cross axis is the inline axis, where a baseline needs text metrics
    // this engine does not have. Flex-start is the documented fallback there.
    ASSERT_FLOAT_EQ(0.0f, child->GetLayout().ComputedX);
}
