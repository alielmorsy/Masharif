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
