#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace _NAMESPACE;

TEST(DimensionTests, wrap_child) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = 100.0f;
    root_child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(root_child0);
    constexpr float  nan=std::numeric_limits<float>::quiet_NaN();
    root->calculate(nan, nan);

    ASSERT_FLOAT_EQ(100.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root->layout().computedHeight);
}

TEST(DimensionTests, max_width) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = 100.0f;
    root_child0->style().modify<Dimensions>().maxWidth = 50.0f;
    root_child0->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child0);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(50.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedHeight);
}

TEST(DimensionTests, min_width) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child0->style().modify<Dimensions>().minWidth = 60.0f;
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<CSSFlex>().flexGrow = 1.0f;
    root->addChild(root_child1);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(60.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(40.0f, root_child1->layout().computedWidth);
}

TEST(DimensionTests, percentage_width) {
    auto root = std::make_shared<Node>();
    root->style().modify<Dimensions>().width = 200.0f;
    root->style().modify<Dimensions>().height = 200.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = CSSValue(50.0f, CSSUnit::PERCENT);
    root_child0->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child0);

    root->calculate(200, 200);

    ASSERT_FLOAT_EQ(100.0f, root_child0->layout().computedWidth);
}
