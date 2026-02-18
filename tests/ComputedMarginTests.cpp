#include <gtest/gtest.h>
#include <masharifcore/Masharif.h>

using namespace masharif;

TEST(ComputedMarginTests, auto_margin) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().position = PositionType::Absolute;
    root->style().modify<Dimensions>().width = 50.0f;
    root->style().modify<Dimensions>().height = 50.0f;

    auto root_child0 = std::make_shared<Node>();
    // Set margin-left: auto using NAN
    root_child0->style().modify<MarginEdge>().left = {NAN, CSSUnit::AUTO};
    root_child0->style().modify<Dimensions>().width = 25.0f;
    root_child0->style().modify<Dimensions>().height = 25.0f;
    root->addChild(root_child0);

    root->calculate(NAN, NAN);

    ASSERT_FLOAT_EQ(0.0f, root->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root->layout().computedY);
    ASSERT_FLOAT_EQ(50.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(50.0f, root->layout().computedHeight);

    ASSERT_FLOAT_EQ(25.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedY);
    ASSERT_FLOAT_EQ(25.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(25.0f, root_child0->layout().computedHeight);
}

TEST(ComputedMarginTests, computed_layout_margin_percentage) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto child = std::make_shared<Node>();
    child->style().modify<Dimensions>().width = 50.0f;
    child->style().modify<Dimensions>().height = 50.0f;
    // simulating YGNodeStyleSetMarginPercent(root, YGEdgeStart, 10);
    // Since we don't have Start, we use Left for LTR test.
    root->addChild(child);
    
    // Set margin-left: 10%
    child->style().modify<MarginEdge>().left = {10.0f, CSSUnit::PERCENT};

    root->calculate(100.0f, 100.0f);

    // 10% of 100 = 10.
    ASSERT_FLOAT_EQ(10.0f, child->layout().computedX);
}

TEST(ComputedMarginTests, margin_side_overrides_horizontal) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto child = std::make_shared<Node>();
    child->style().modify<Dimensions>().width = 10.0f;
    child->style().modify<Dimensions>().height = 10.0f;
    root->addChild(child);

    child->style().modify<MarginEdge>().left = 10.0f;
    child->style().modify<MarginEdge>().right = 10.0f;
    
    // Override Left
    child->style().modify<MarginEdge>().left = 20.0f;

    root->calculate(100.0f, 100.0f);

    ASSERT_FLOAT_EQ(20.0f, child->layout().computedX);
}
