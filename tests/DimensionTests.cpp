#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(DimensionTests, wrap_child) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(root_child0);
    constexpr float  nan=std::numeric_limits<float>::quiet_NaN();
    root->Calculate(nan, nan);

    ASSERT_FLOAT_EQ(100.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root->GetLayout().ComputedHeight);
}

TEST(DimensionTests, max_width) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root_child0->GetStyle().Modify<Dimensions>().MaxWidth = 50.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child0);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(50.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(10.0f, root_child0->GetLayout().ComputedHeight);
}

TEST(DimensionTests, min_width) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root_child0->GetStyle().Modify<Dimensions>().MinWidth = 60.0f;
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root->AddChild(root_child1);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(60.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(40.0f, root_child1->GetLayout().ComputedWidth);
}

TEST(DimensionTests, percentage_width) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<Dimensions>().Width = CSSValue(50.0f, CSSUnit::Percent);
    root_child0->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child0);

    root->Calculate(200, 200);

    ASSERT_FLOAT_EQ(100.0f, root_child0->GetLayout().ComputedWidth);
}
