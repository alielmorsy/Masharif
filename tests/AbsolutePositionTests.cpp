#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(AbsolutePositionTests, absolute_layout_width_height_start_top) {
    auto root = std::make_shared<Node>();
    root->setPositionType(PositionType::Absolute);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->setPositionType(PositionType::Absolute);
    root_child0->style().modify<PositionEdge>().left = 10.0f;
    root_child0->style().modify<PositionEdge>().top = 10.0f;
    root_child0->style().modify<Dimensions>().width = 10.0f;
    root_child0->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child0);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedY);
    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedHeight);
}

TEST(AbsolutePositionTests, absolute_layout_width_height_left_auto_right) {
    auto root = std::make_shared<Node>();
    root->setPositionType(PositionType::Absolute);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->setPositionType(PositionType::Absolute);
    root_child0->style().modify<PositionEdge>().left = NaN; // Auto
    root_child0->style().modify<PositionEdge>().right = 10.0f;
    root_child0->style().modify<Dimensions>().width = 10.0f;
    root_child0->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child0);

    root->calculate(100, 100);

    // root width (100) - child width (10) - right (10) = 80
    ASSERT_FLOAT_EQ(80.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(0.0f, root_child0->layout().computedY);
}

TEST(AbsolutePositionTests, absolute_layout_start_top_end_bottom) {
    auto root = std::make_shared<Node>();
    root->setPositionType(PositionType::Absolute);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->setPositionType(PositionType::Absolute);
    root_child0->style().modify<PositionEdge>().left = 10.0f;
    root_child0->style().modify<PositionEdge>().top = 10.0f;
    root_child0->style().modify<PositionEdge>().right = 10.0f;
    root_child0->style().modify<PositionEdge>().bottom = 10.0f;
    root->addChild(root_child0);

    root->calculate(100, 100);

    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedY);
    ASSERT_FLOAT_EQ(80.0f, root_child0->layout().computedWidth);
    ASSERT_FLOAT_EQ(80.0f, root_child0->layout().computedHeight);
}

TEST(AbsolutePositionTests, absolute_layout_align_items_and_justify_content_center) {
    auto root = std::make_shared<Node>();
    root->style().modify<CSSFlex>().justifyContent = JustifyContent::FlexCenter;
    root->style().modify<CSSFlex>().alignItems = AlignItems::FlexCenter;
    root->setPositionType(PositionType::Absolute);
    root->style().modify<Dimensions>().width = 110.0f;
    root->style().modify<Dimensions>().height = 100.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->setPositionType(PositionType::Absolute);
    root_child0->style().modify<Dimensions>().width = 60.0f;
    root_child0->style().modify<Dimensions>().height = 40.0f;
    root->addChild(root_child0);

    root->calculate(110, 100);

    // (110 - 60) / 2 = 25
    // (100 - 40) / 2 = 30
    ASSERT_FLOAT_EQ(25.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(30.0f, root_child0->layout().computedY);
}
