#include <gtest/gtest.h>


#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(MasharifTest, assert_default_values) {
    auto root = std::make_shared<Node>();

    ASSERT_EQ(0, root->Children().size());

    // Check initial layout values
    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(0.0f, root->GetLayout().ComputedHeight);

    // Initial style should be Block display by default (from Node constructor)
    ASSERT_EQ(OuterDisplay::Block, root->GetStyle().GetDimensions().Display);
}

TEST(MasharifTest, simple_layout) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root->GetStyle().Modify<Dimensions>().Height = 500.0f;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child0);

    root->Calculate(500, 500);

    ASSERT_FLOAT_EQ(500.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(500.0f, root->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(100.0f, child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, child0->GetLayout().ComputedHeight);
}

TEST(MasharifTest, flex_layout_basics) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root->GetStyle().Modify<Dimensions>().Height = 500.0f;
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child1->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child1);

    root->Calculate(500, 500);

    ASSERT_FLOAT_EQ(100.0f, child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(0.0f, child0->GetLayout().ComputedX);

    ASSERT_FLOAT_EQ(100.0f, child1->GetLayout().ComputedWidth);
    // In a row, child1 should be after child0
    ASSERT_FLOAT_EQ(100.0f, child1->GetLayout().ComputedX);
}

TEST(MasharifTest, flex_direction_column) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root->GetStyle().Modify<Dimensions>().Height = 500.0f;
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child1->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child1);

    root->Calculate(500, 500);

    ASSERT_FLOAT_EQ(100.0f, child0->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(0.0f, child0->GetLayout().ComputedY);

    ASSERT_FLOAT_EQ(100.0f, child1->GetLayout().ComputedHeight);
    // In a column, child1 should be below child0
    ASSERT_FLOAT_EQ(100.0f, child1->GetLayout().ComputedY);
}

TEST(MasharifTest, justify_content_center) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::FlexCenter;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child0);

    root->Calculate(500, 100);

    // root is 500 wide, child is 100 wide. 
    // center means (500 - 100) / 2 = 200.
    ASSERT_FLOAT_EQ(200.0f, child0->GetLayout().ComputedX);
}

TEST(MasharifTest, align_items_center) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root->GetStyle().Modify<Dimensions>().Height = 500.0f;
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child0);

    root->Calculate(500, 500);

    // root is 500 high, child is 100 high.
    // center means (500 - 100) / 2 = 200.
    ASSERT_FLOAT_EQ(200.0f, child0->GetLayout().ComputedY);
}

TEST(MasharifTest, flex_grow_test) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->GetStyle().Modify<Dimensions>().Height = 100.0f;
    child1->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root->AddChild(child1);

    root->Calculate(500, 100);

    // child0 is 100. Remaining space is 400.
    // child1 flexGrow is 1, so it should take all 400.
    ASSERT_FLOAT_EQ(100.0f, child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(400.0f, child1->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, child1->GetLayout().ComputedX);
}

TEST(MasharifTest, padding_test) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root->GetStyle().Modify<Dimensions>().Height = 500.0f;
    root->GetStyle().Modify<PaddingEdge>().Left = 20.0f;
    root->GetStyle().Modify<PaddingEdge>().Top = 10.0f;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(child0);

    root->Calculate(500, 500);

    // child0 should be offset by padding
    ASSERT_FLOAT_EQ(20.0f, child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(10.0f, child0->GetLayout().ComputedY);
}
