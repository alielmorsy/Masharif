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
