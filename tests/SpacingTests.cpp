#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;
constexpr float NaN = std::numeric_limits<float>::quiet_NaN();
TEST(SpacingTests, padding_no_size) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<PaddingEdge>().Left = 10.0f;
    root->GetStyle().Modify<PaddingEdge>().Top = 10.0f;
    root->GetStyle().Modify<PaddingEdge>().Right = 10.0f;
    root->GetStyle().Modify<PaddingEdge>().Bottom = 10.0f;

    root->Calculate(NaN, NaN);

    ASSERT_FLOAT_EQ(20.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(20.0f, root->GetLayout().ComputedHeight);
}

TEST(SpacingTests, padding_container_match_child) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<PaddingEdge>().Left = 10.0f;
    root->GetStyle().Modify<PaddingEdge>().Top = 10.0f;
    root->GetStyle().Modify<PaddingEdge>().Right = 10.0f;
    root->GetStyle().Modify<PaddingEdge>().Bottom = 10.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = 10.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child0);

    root->Calculate(NaN, NaN);

    ASSERT_FLOAT_EQ(30.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(30.0f, root->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedY);
}

// gh#6 -- CSS resolves a percentage padding against the CONTAINING BLOCK's width on all four
// sides, never against the box's own width. `pct` is 120 wide inside a 300-wide containing block,
// so 10% is 30px -- not 12.
TEST(SpacingTests, percentage_padding_resolves_against_containing_block_width) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 160.0f;

    auto pct = std::make_shared<Node>();
    pct->SetDisplay(OuterDisplay::Flex);
    pct->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    pct->GetStyle().Modify<Dimensions>().Width = 120.0f;
    pct->GetStyle().Modify<Dimensions>().Height = 80.0f;
    pct->GetStyle().Modify<PaddingEdge>().Left = CSSValue(10.0f, CSSUnit::Percent);
    pct->GetStyle().Modify<PaddingEdge>().Top = CSSValue(10.0f, CSSUnit::Percent);
    root->AddChild(pct);

    auto inner = std::make_shared<Node>();
    inner->SetDisplay(OuterDisplay::Flex);
    inner->GetStyle().Modify<Dimensions>().Height = 20.0f;
    pct->AddChild(inner);

    root->Calculate(300, 160);

    ASSERT_FLOAT_EQ(120.0f, pct->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(80.0f, pct->GetLayout().ComputedHeight);

    // The content box is 120-30 = 90 wide, offset 30px in from both the left and the top edge.
    ASSERT_FLOAT_EQ(30.0f, inner->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(30.0f, inner->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(90.0f, inner->GetLayout().ComputedWidth);
}
