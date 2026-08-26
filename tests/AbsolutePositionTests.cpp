#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;

// Ported from the pre-rename API (`setPositionType`, `style().modify<PositionEdge>()`,
// `layout().computedX`, `calculate`), which stopped compiling when those names changed and left
// this whole file — listed in tests/CMakeLists.txt — silently unbuildable, taking the entire
// unit_tests target down with it. Test intent and expected values are unchanged; only the API
// spelling is. `PositionEdge` became the Top/Right/Bottom/Left insets on `Dimensions`.

TEST(AbsolutePositionTests, absolute_layout_width_height_start_top) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    root_child0->GetStyle().Modify<Dimensions>().Left = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Top = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child0);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedHeight);
}

TEST(AbsolutePositionTests, absolute_layout_width_height_left_auto_right) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    // Left stays AUTO (the default) so the box anchors to its Right inset — the case that
    // silently broke while insets defaulted to 0px instead of auto.
    root_child0->GetStyle().Modify<Dimensions>().Right = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child0);

    root->Calculate(100, 100);

    // root width (100) - child width (10) - right (10) = 80
    ASSERT_FLOAT_EQ(80.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedY);
}

TEST(AbsolutePositionTests, absolute_layout_start_top_end_bottom) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    root_child0->GetStyle().Modify<Dimensions>().Left = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Top = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Right = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Bottom = 10.0f;
    root->AddChild(root_child0);

    root->Calculate(100, 100);

    // An AUTO size pinned by BOTH insets fills the gap between them: 100 - 10 - 10 = 80.
    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(80.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(80.0f, root_child0->GetLayout().ComputedHeight);
}

TEST(AbsolutePositionTests, absolute_layout_align_items_and_justify_content_center) {
    auto root = std::make_shared<Node>();
    // A flex container is what contributes alignment to an auto-inset child's static position
    // (see Node::PositionOutOfFlowChild) — the original test predates the display split and set
    // only the flex properties.
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexCenter;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    root->GetStyle().Modify<Dimensions>().Width = 110.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    root_child0->GetStyle().Modify<Dimensions>().Width = 60.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(root_child0);

    root->Calculate(110, 100);

    // All four insets AUTO, so the child lands at the static position the container's
    // justify-content/align-items imply: (110 - 60) / 2 = 25, (100 - 40) / 2 = 30.
    ASSERT_FLOAT_EQ(25.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(30.0f, root_child0->GetLayout().ComputedY);
}

/// CSS 2.1 10.3.7 / 10.6.4: `left`, `width` and `right` (and the vertical trio) all non-auto
/// over-constrains the box. With `auto` margins on the axis the leftover is split between them,
/// which is the standard centring idiom -- the engine used to leave the box at its inset origin.
TEST(AbsolutePositionTests, absolute_over_constrained_auto_margins_centre) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto child = std::make_shared<Node>();
    auto &dims = child->GetStyle().Modify<Dimensions>();
    dims.Position = PositionType::Absolute;
    dims.Top = dims.Right = dims.Bottom = dims.Left = 0.0f;
    dims.Width = 120.0f;
    dims.Height = 60.0f;
    auto &margin = child->GetStyle().Modify<MarginEdge>();
    margin.Left = margin.Right = margin.Top = margin.Bottom = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(child);

    root->Calculate(300, 200);

    // (300 - 120) / 2 = 90, (200 - 60) / 2 = 70.
    ASSERT_FLOAT_EQ(90.0f, child->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(70.0f, child->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(120.0f, child->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(60.0f, child->GetLayout().ComputedHeight);
}

/// One `auto` margin on an over-constrained axis takes the WHOLE leftover, which pins the box to
/// the opposite inset. The non-auto margin on the other side is part of the constraint equation,
/// so it comes off the leftover first.
TEST(AbsolutePositionTests, absolute_over_constrained_single_auto_margin_absorbs_leftover) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto child = std::make_shared<Node>();
    auto &dims = child->GetStyle().Modify<Dimensions>();
    dims.Position = PositionType::Absolute;
    dims.Left = 10.0f;
    dims.Right = 20.0f;
    dims.Top = 0.0f;
    dims.Bottom = 0.0f;
    dims.Width = 100.0f;
    dims.Height = 50.0f;
    auto &margin = child->GetStyle().Modify<MarginEdge>();
    margin.Left = CSSValue(0.0f, CSSUnit::Auto); // absorbs everything left over
    margin.Right = 30.0f; // part of the equation, not leftover
    margin.Top = CSSValue(0.0f, CSSUnit::Auto);
    margin.Bottom = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(child);

    root->Calculate(300, 200);

    // 300 - 10 - 20 - 100 - 30 = 140 of leftover, all of it to margin-left: 10 + 140 = 150.
    ASSERT_FLOAT_EQ(150.0f, child->GetLayout().ComputedX);
    // Vertical is the both-auto split: (200 - 50) / 2 = 75.
    ASSERT_FLOAT_EQ(75.0f, child->GetLayout().ComputedY);
}

/// An `auto` margin only takes leftover when the axis is over-constrained. With one inset auto
/// there is nothing over-constrained to redistribute, so CSS resolves the margin to 0 and the box
/// stays on its single anchor.
TEST(AbsolutePositionTests, absolute_auto_margin_is_zero_without_both_insets) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto child = std::make_shared<Node>();
    auto &dims = child->GetStyle().Modify<Dimensions>();
    dims.Position = PositionType::Absolute;
    dims.Left = 10.0f; // Right stays AUTO
    dims.Top = 10.0f; // Bottom stays AUTO
    dims.Width = 120.0f;
    dims.Height = 60.0f;
    auto &margin = child->GetStyle().Modify<MarginEdge>();
    margin.Left = margin.Right = margin.Top = margin.Bottom = CSSValue(0.0f, CSSUnit::Auto);
    root->AddChild(child);

    root->Calculate(300, 200);

    ASSERT_FLOAT_EQ(10.0f, child->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(10.0f, child->GetLayout().ComputedY);
}

/// CSS 2.1 10.3.7 / 10.6.4: the box's own margin is part of the inset constraint equation, so the
/// leading margin insets the border box from the inset it is anchored to. The engine used to
/// position an out-of-flow box off its insets alone and ignore its margins entirely.
TEST(AbsolutePositionTests, absolute_start_anchored_box_is_inset_by_its_leading_margin) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto child = std::make_shared<Node>();
    auto &dims = child->GetStyle().Modify<Dimensions>();
    dims.Position = PositionType::Absolute;
    dims.Left = 10.0f;
    dims.Top = 20.0f;
    dims.Width = 100.0f;
    dims.Height = 50.0f;
    auto &margin = child->GetStyle().Modify<MarginEdge>();
    margin.Left = 5.0f;
    margin.Top = 7.0f;
    root->AddChild(child);

    root->Calculate(300, 200);

    ASSERT_FLOAT_EQ(15.0f, child->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(27.0f, child->GetLayout().ComputedY);
}

/// The mirror image: a box anchored by its END inset has its TRAILING margin between it and that
/// inset, so the margin moves it back towards the start edge.
TEST(AbsolutePositionTests, absolute_end_anchored_box_is_inset_by_its_trailing_margin) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto child = std::make_shared<Node>();
    auto &dims = child->GetStyle().Modify<Dimensions>();
    dims.Position = PositionType::Absolute;
    dims.Right = 10.0f; // Left stays AUTO
    dims.Bottom = 20.0f; // Top stays AUTO
    dims.Width = 100.0f;
    dims.Height = 50.0f;
    auto &margin = child->GetStyle().Modify<MarginEdge>();
    margin.Right = 5.0f;
    margin.Bottom = 7.0f;
    root->AddChild(child);

    root->Calculate(300, 200);

    // 300 - 10 - 5 - 100 = 185, 200 - 20 - 7 - 50 = 123.
    ASSERT_FLOAT_EQ(185.0f, child->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(123.0f, child->GetLayout().ComputedY);
}

/// With no insets at all the box lands at its static position, which CSS defines as where its
/// MARGIN edge would have gone in flow: the container's alignment distributes the margin box, and
/// the leading margin then insets the border box from that.
TEST(AbsolutePositionTests, absolute_static_position_aligns_the_margin_box) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexCenter;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;
    root->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto child = std::make_shared<Node>();
    auto &dims = child->GetStyle().Modify<Dimensions>();
    dims.Position = PositionType::Absolute; // every inset AUTO
    dims.Width = 100.0f;
    dims.Height = 50.0f;
    auto &margin = child->GetStyle().Modify<MarginEdge>();
    margin.Left = 10.0f;
    margin.Right = 30.0f;
    margin.Top = 8.0f;
    margin.Bottom = 12.0f;
    root->AddChild(child);

    root->Calculate(300, 200);

    // Margin box 140x70 centred in 300x200 -> free 160x130, half of it 80x65, then the leading
    // margin: 80 + 10 = 90 and 65 + 8 = 73.
    ASSERT_FLOAT_EQ(90.0f, child->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(73.0f, child->GetLayout().ComputedY);
}
