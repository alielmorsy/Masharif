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
