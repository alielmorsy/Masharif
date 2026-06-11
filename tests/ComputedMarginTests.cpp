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
