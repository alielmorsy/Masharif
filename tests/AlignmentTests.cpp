#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace _NAMESPACE;

TEST(AlignmentTests, justify_content_row_flex_start) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = 10.0f;
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<Dimensions>().width = 10.0f;
    root->addChild(root_child1);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(10.0f, root_child1->layout().computedX);
}

TEST(AlignmentTests, justify_content_row_flex_end) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<CSSFlex>().justifyContent = JustifyContent::FlexEnd;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = 10.0f;
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<Dimensions>().width = 10.0f;
    root->addChild(root_child1);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(80.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(90.0f, root_child1->layout().computedX);
}

TEST(AlignmentTests, justify_content_row_center) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<CSSFlex>().justifyContent = JustifyContent::FlexCenter;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = 10.0f;
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<Dimensions>().width = 10.0f;
    root->addChild(root_child1);

    root->calculate(100, 100);

    // (100 - 20) / 2 = 40
    ASSERT_FLOAT_EQ(40.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(50.0f, root_child1->layout().computedX);
}

TEST(AlignmentTests, align_items_stretch) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;
    root->style().modify<CSSFlex>().alignItems = AlignItems::Stretch;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child0);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(100.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedHeight);
}

TEST(AlignmentTests, align_items_center) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;
    root->style().modify<CSSFlex>().alignItems = AlignItems::FlexCenter;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = 10.0f;
    root_child0->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child0);

    root->calculate(100, 100);

    // (100 - 10) / 2 = 45
    ASSERT_FLOAT_EQ(45.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedY);
}

TEST(AlignmentTests, align_self_flex_end) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = 10.0f;
    root_child0->style().modify<Dimensions>().height = 10.0f;
    root_child0->style().modify<CSSFlex>().alignSelf = AlignItems::FlexEnd;
    root->addChild(root_child0);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(90.0f, root_child0->layout().computedX);
}
