#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(GapTests, column_gap_flexible) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<Dimensions>().width = CSSValue(80.0f);
    root->style().modify<Dimensions>().height = CSSValue(100.0f);
    root->style().modify<CSSFlex>().gap.column = CSSValue(10.0f);
    root->style().modify<CSSFlex>().gap.row = CSSValue(20.0f);

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child0->style().modify<CSSFlex>().flexShrink = 1.0f;
    root_child0->style().modify<CSSFlex>().flexBasis = CSSValue(0.0f, CSSUnit::PERCENT);
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child1->style().modify<CSSFlex>().flexShrink = 1.0f;
    root_child1->style().modify<CSSFlex>().flexBasis = CSSValue(0.0f, CSSUnit::PERCENT);
    root->addChild(root_child1);

    auto root_child2 = std::make_shared<Node>();
    root_child2->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child2->style().modify<CSSFlex>().flexShrink = 1.0f;
    root_child2->style().modify<CSSFlex>().flexBasis = CSSValue(0.0f, CSSUnit::PERCENT);
    root->addChild(root_child2);

    root->calculate(80.0f, 100.0f);

    ASSERT_FLOAT_EQ(0.0f, root->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root->layout().computedY);
    ASSERT_FLOAT_EQ(80.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root->layout().computedHeight);

    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedY);
    ASSERT_FLOAT_EQ(20.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child0->layout().computedHeight);

    ASSERT_FLOAT_EQ(30.0f, root_child1->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child1->layout().computedY);
    ASSERT_FLOAT_EQ(20.0f, root_child1->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child1->layout().computedHeight);

    ASSERT_FLOAT_EQ(60.0f, root_child2->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child2->layout().computedY);
    ASSERT_FLOAT_EQ(20.0f, root_child2->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child2->layout().computedHeight);
}

TEST(GapTests, column_gap_inflexible) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<Dimensions>().width = CSSValue(80.0f);
    root->style().modify<Dimensions>().height = CSSValue(100.0f);
    root->style().modify<CSSFlex>().gap.column = CSSValue(10.0f);

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = CSSValue(20.0f);
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<Dimensions>().width = CSSValue(20.0f);
    root->addChild(root_child1);

    auto root_child2 = std::make_shared<Node>();
    root_child2->style().modify<Dimensions>().width = CSSValue(20.0f);
    root->addChild(root_child2);

    root->calculate(80.0f, 100.0f);

    ASSERT_FLOAT_EQ(0.0f, root->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root->layout().computedY);
    ASSERT_FLOAT_EQ(80.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root->layout().computedHeight);

    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedY);
    ASSERT_FLOAT_EQ(20.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child0->layout().computedHeight);

    ASSERT_FLOAT_EQ(30.0f, root_child1->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child1->layout().computedY);
    ASSERT_FLOAT_EQ(20.0f, root_child1->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child1->layout().computedHeight);

    ASSERT_FLOAT_EQ(60.0f, root_child2->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child2->layout().computedY);
    ASSERT_FLOAT_EQ(20.0f, root_child2->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child2->layout().computedHeight);
}
