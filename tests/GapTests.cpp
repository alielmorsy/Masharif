#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(GapTests, column_gap_flexible) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(80.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f);
    root->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(10.0f);
    root->GetStyle().Modify<CSSFlex>().Gaps.Row = CSSValue(20.0f);

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root_child0->GetStyle().Modify<CSSFlex>().FlexShrink = 1.0f;
    root_child0->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(0.0f, CSSUnit::Percent);
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root_child1->GetStyle().Modify<CSSFlex>().FlexShrink = 1.0f;
    root_child1->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(0.0f, CSSUnit::Percent);
    root->AddChild(root_child1);

    auto root_child2 = std::make_shared<Node>();
    root_child2->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root_child2->GetStyle().Modify<CSSFlex>().FlexShrink = 1.0f;
    root_child2->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(0.0f, CSSUnit::Percent);
    root->AddChild(root_child2);

    root->Calculate(80.0f, 100.0f);

    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(80.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(20.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child0->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(30.0f, root_child1->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child1->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(20.0f, root_child1->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child1->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(60.0f, root_child2->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child2->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(20.0f, root_child2->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child2->GetLayout().ComputedHeight);
}

TEST(GapTests, column_gap_inflexible) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(80.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f);
    root->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(10.0f);

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = CSSValue(20.0f);
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<Dimensions>().Width = CSSValue(20.0f);
    root->AddChild(root_child1);

    auto root_child2 = std::make_shared<Node>();
    root_child2->GetStyle().Modify<Dimensions>().Width = CSSValue(20.0f);
    root->AddChild(root_child2);

    root->Calculate(80.0f, 100.0f);

    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(80.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(20.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child0->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(30.0f, root_child1->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child1->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(20.0f, root_child1->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child1->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(60.0f, root_child2->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root_child2->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(20.0f, root_child2->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child2->GetLayout().ComputedHeight);
}

// --- gh#2: justify-content free space must exclude gaps and main-axis item margins ---------------
//
// `remainingSpace` in PositionLineOnMainAxis is what every justify-content branch divides up. Gaps
// and the items' own non-auto main-axis margins are consumed by the line, so they are not free
// space. Counting them as free lands every value other than flex-start off by the total gap (or a
// fraction of it), and the line overflows its own container.

TEST(GapTests, justify_flex_end_excludes_gaps_from_free_space) {
    auto root = std::make_shared<Node>(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 400.0f;
    root->GetStyle().Modify<Dimensions>().Height = 50.0f;
    root->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(20.0f);
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexEnd;

    std::shared_ptr<Node> items[3];
    for (auto &item: items) {
        item = std::make_shared<Node>();
        item->GetStyle().Modify<Dimensions>().Width = 100.0f;
        root->AddChild(item);
    }

    root->Calculate(400.0f, 50.0f);

    // 400 - (3 * 100) - (2 * 20) = 60 free, all of it before the first item.
    ASSERT_FLOAT_EQ(60.0f, items[0]->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(180.0f, items[1]->GetLayout().ComputedX);
    // The last item ends flush with the container's content edge, which is what flex-end means.
    ASSERT_FLOAT_EQ(300.0f, items[2]->GetLayout().ComputedX);
}

TEST(GapTests, justify_center_excludes_gaps_from_free_space) {
    auto root = std::make_shared<Node>(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 400.0f;
    root->GetStyle().Modify<Dimensions>().Height = 50.0f;
    root->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(20.0f);
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexCenter;

    std::shared_ptr<Node> items[3];
    for (auto &item: items) {
        item = std::make_shared<Node>();
        item->GetStyle().Modify<Dimensions>().Width = 100.0f;
        root->AddChild(item);
    }

    root->Calculate(400.0f, 50.0f);

    // Half of the 60px free space on each side: the line is 340 wide, centred in 400.
    ASSERT_FLOAT_EQ(30.0f, items[0]->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(150.0f, items[1]->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(270.0f, items[2]->GetLayout().ComputedX);
}

TEST(GapTests, justify_space_between_adds_to_the_declared_gap) {
    auto root = std::make_shared<Node>(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 400.0f;
    root->GetStyle().Modify<Dimensions>().Height = 50.0f;
    root->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(20.0f);
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::SpaceBetween;

    std::shared_ptr<Node> items[3];
    for (auto &item: items) {
        item = std::make_shared<Node>();
        item->GetStyle().Modify<Dimensions>().Width = 100.0f;
        root->AddChild(item);
    }

    root->Calculate(400.0f, 50.0f);

    // 60 free split over the 2 gaps -> each gap becomes 20 + 30 = 50.
    ASSERT_FLOAT_EQ(0.0f, items[0]->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(150.0f, items[1]->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(300.0f, items[2]->GetLayout().ComputedX);
}

TEST(GapTests, justify_column_center_excludes_row_gaps_from_free_space) {
    auto root = std::make_shared<Node>(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 400.0f;
    root->GetStyle().Modify<CSSFlex>().Gaps.Row = CSSValue(20.0f);
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexCenter;

    std::shared_ptr<Node> items[3];
    for (auto &item: items) {
        item = std::make_shared<Node>();
        item->GetStyle().Modify<Dimensions>().Height = 100.0f;
        root->AddChild(item);
    }

    root->Calculate(100.0f, 400.0f);

    ASSERT_FLOAT_EQ(30.0f, items[0]->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(150.0f, items[1]->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(270.0f, items[2]->GetLayout().ComputedY);
}

TEST(GapTests, justify_flex_end_excludes_item_margins_from_free_space) {
    auto root = std::make_shared<Node>(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 400.0f;
    root->GetStyle().Modify<Dimensions>().Height = 50.0f;
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexEnd;

    auto c0 = std::make_shared<Node>();
    c0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    c0->GetStyle().Modify<MarginEdge>().Right = CSSValue(40.0f);
    root->AddChild(c0);

    auto c1 = std::make_shared<Node>();
    c1->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->AddChild(c1);

    root->Calculate(400.0f, 50.0f);

    // 400 - 200 - 40 = 160 free. c1's right edge lands on the container's, not 40px past it.
    ASSERT_FLOAT_EQ(160.0f, c0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(300.0f, c1->GetLayout().ComputedX);
}
