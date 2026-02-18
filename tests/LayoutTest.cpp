#include <gtest/gtest.h>


#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(MasharifTest, assert_default_values) {
    auto root = std::make_shared<Node>();

    ASSERT_EQ(0, root->children.size());

    // Check initial layout values
    ASSERT_FLOAT_EQ(0.0f, root->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root->layout().computedY);
    ASSERT_FLOAT_EQ(0.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(0.0f, root->layout().computedHeight);

    // Initial style should be Block display by default (from Node constructor)
    ASSERT_EQ(OuterDisplay::Block, root->style().dimensions().display);
}

TEST(MasharifTest, simple_layout) {
    auto root = std::make_shared<Node>();
    root->style().modify<Dimensions>().width = 500.0f;
    root->style().modify<Dimensions>().height = 500.0f;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 100.0f;
    child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child0);

    root->calculate(500, 500);

    ASSERT_FLOAT_EQ(500.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(500.0f, root->layout().computedHeight);

    ASSERT_FLOAT_EQ(100.0f, child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, child0->layout().computedHeight);
}

TEST(MasharifTest, flex_layout_basics) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 500.0f;
    root->style().modify<Dimensions>().height = 500.0f;
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 100.0f;
    child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->style().modify<Dimensions>().width = 100.0f;
    child1->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child1);

    root->calculate(500, 500);

    ASSERT_FLOAT_EQ(100.0f, child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(0.0f, child0->layout().computedX);

    ASSERT_FLOAT_EQ(100.0f, child1->layout().computedWidth);
    // In a row, child1 should be after child0
    ASSERT_FLOAT_EQ(100.0f, child1->layout().computedX);
}

TEST(MasharifTest, flex_direction_column) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 500.0f;
    root->style().modify<Dimensions>().height = 500.0f;
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 100.0f;
    child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->style().modify<Dimensions>().width = 100.0f;
    child1->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child1);

    root->calculate(500, 500);

    ASSERT_FLOAT_EQ(100.0f, child0->layout().computedHeight);
    ASSERT_FLOAT_EQ(0.0f, child0->layout().computedY);

    ASSERT_FLOAT_EQ(100.0f, child1->layout().computedHeight);
    // In a column, child1 should be below child0
    ASSERT_FLOAT_EQ(100.0f, child1->layout().computedY);
}

TEST(MasharifTest, justify_content_center) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 500.0f;
    root->style().modify<Dimensions>().height = 100.0f;
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<CSSFlex>().justifyContent = JustifyContent::FlexCenter;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 100.0f;
    child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child0);

    root->calculate(500, 100);

    // root is 500 wide, child is 100 wide. 
    // center means (500 - 100) / 2 = 200.
    ASSERT_FLOAT_EQ(200.0f, child0->layout().computedX);
}

TEST(MasharifTest, align_items_center) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 500.0f;
    root->style().modify<Dimensions>().height = 500.0f;
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<CSSFlex>().alignItems = AlignItems::FlexCenter;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 100.0f;
    child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child0);

    root->calculate(500, 500);

    // root is 500 high, child is 100 high.
    // center means (500 - 100) / 2 = 200.
    ASSERT_FLOAT_EQ(200.0f, child0->layout().computedY);
}

TEST(MasharifTest, flex_grow_test) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 500.0f;
    root->style().modify<Dimensions>().height = 100.0f;
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 100.0f;
    child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->style().modify<Dimensions>().height = 100.0f;
    child1->style().modify<CSSFlex>().flexGrow = 1.0f;
    root->addChild(child1);

    root->calculate(500, 100);

    // child0 is 100. Remaining space is 400.
    // child1 flexGrow is 1, so it should take all 400.
    ASSERT_FLOAT_EQ(100.0f, child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(400.0f, child1->layout().computedWidth);
    ASSERT_FLOAT_EQ(100.0f, child1->layout().computedX);
}

TEST(MasharifTest, padding_test) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 500.0f;
    root->style().modify<Dimensions>().height = 500.0f;
    root->style().modify<PaddingEdge>().left = 20.0f;
    root->style().modify<PaddingEdge>().top = 10.0f;

    auto child0 = std::make_shared<Node>();
    child0->style().modify<Dimensions>().width = 100.0f;
    child0->style().modify<Dimensions>().height = 100.0f;
    root->addChild(child0);

    root->calculate(500, 500);

    // child0 should be offset by padding
    ASSERT_FLOAT_EQ(20.0f, child0->layout().computedX);
    ASSERT_FLOAT_EQ(10.0f, child0->layout().computedY);
}
