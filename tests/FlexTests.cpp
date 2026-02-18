#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(FlexTests, flex_basis_flex_grow_column) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child0->style().modify<CSSFlex>().flexBasis = 50.0f;
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<CSSFlex>().flexGrow = 1.0f;
    root->addChild(root_child1);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(100.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root->layout().computedHeight);

    ASSERT_FLOAT_EQ(100.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(75.0f, root_child0->layout().computedHeight);
    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedY);

    ASSERT_FLOAT_EQ(100.0f, root_child1->layout().computedWidth);
    ASSERT_FLOAT_EQ(25.0f, root_child1->layout().computedHeight);
    ASSERT_FLOAT_EQ(75.0f, root_child1->layout().computedY);
}

TEST(FlexTests, flex_shrink_flex_grow_row) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<Dimensions>().width = 500.0f;
    root->style().modify<Dimensions>().height = 500.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<CSSFlex>().flexShrink = 1.0f;
    root_child0->style().modify<Dimensions>().width = 500.0f;
    root_child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<CSSFlex>().flexShrink = 1.0f;
    root_child1->style().modify<Dimensions>().width = 500.0f;
    root_child1->style().modify<Dimensions>().height = 100.0f;
    root->addChild(root_child1);

    root->calculate(500, 500);

    ASSERT_FLOAT_EQ(500.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(500.0f, root->layout().computedHeight);

    ASSERT_FLOAT_EQ(250.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child0->layout().computedHeight);
    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedX);

    ASSERT_FLOAT_EQ(250.0f, root_child1->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child1->layout().computedHeight);
    ASSERT_FLOAT_EQ(250.0f, root_child1->layout().computedX);
}

TEST(FlexTests, flex_basis_overrides_main_size) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child0->style().modify<CSSFlex>().flexBasis = 50.0f;
    root_child0->style().modify<Dimensions>().height = 20.0f;
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child1->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child1);

    auto root_child2 = std::make_shared<Node>();
    root_child2->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child2->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child2);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(100.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(60.0f, root_child0->layout().computedHeight);

    ASSERT_FLOAT_EQ(100.0f, root_child1->layout().computedWidth);
    ASSERT_FLOAT_EQ(20.0f, root_child1->layout().computedHeight);
    ASSERT_FLOAT_EQ(60.0f, root_child1->layout().computedY);

    ASSERT_FLOAT_EQ(100.0f, root_child2->layout().computedWidth);
    ASSERT_FLOAT_EQ(20.0f, root_child2->layout().computedHeight);
    ASSERT_FLOAT_EQ(80.0f, root_child2->layout().computedY);
}

TEST(FlexTests, flex_direction_column_reverse) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::ColumnReverse;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 100.0f;
    child0->style().modify<Dimensions>().height = 20.0f;
    root->addChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->style().modify<Dimensions>().width = 100.0f;
    child1->style().modify<Dimensions>().height = 20.0f;
    root->addChild(child1);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(80.0f, child0->layout().computedY);
    ASSERT_FLOAT_EQ(60.0f, child1->layout().computedY);
}

TEST(FlexTests, flex_wrap_wrap) {
    auto root = std::make_shared<Node>();
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<CSSFlex>().wrap = FlexWrap::Wrap;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 60.0f;
    child0->style().modify<Dimensions>().height = 40.0f;
    root->addChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->style().modify<Dimensions>().width = 60.0f;
    child1->style().modify<Dimensions>().height = 40.0f;
    root->addChild(child1);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(0.0f, child0->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, child0->layout().computedY);

    ASSERT_FLOAT_EQ(0.0f, child1->layout().computedX);
    ASSERT_FLOAT_EQ(40.0f, child1->layout().computedY);
}
