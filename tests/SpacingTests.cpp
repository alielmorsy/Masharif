#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;
constexpr float NaN = std::numeric_limits<float>::quiet_NaN();
TEST(SpacingTests, padding_no_size) {
    auto root = std::make_shared<Node>();
    root->style().modify<PaddingEdge>().left = 10.0f;
    root->style().modify<PaddingEdge>().top = 10.0f;
    root->style().modify<PaddingEdge>().right = 10.0f;
    root->style().modify<PaddingEdge>().bottom = 10.0f;

    root->calculate(NaN, NaN);

    ASSERT_FLOAT_EQ(20.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(20.0f, root->layout().computedHeight);
}

TEST(SpacingTests, padding_container_match_child) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<PaddingEdge>().left = 10.0f;
    root->style().modify<PaddingEdge>().top = 10.0f;
    root->style().modify<PaddingEdge>().right = 10.0f;
    root->style().modify<PaddingEdge>().bottom = 10.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<Dimensions>().width = 10.0f;
    root_child0->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child0);

    root->calculate(NaN, NaN);

    ASSERT_FLOAT_EQ(30.0f, root->layout().computedWidth);
    ASSERT_FLOAT_EQ(30.0f, root->layout().computedHeight);

    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedX);
    ASSERT_FLOAT_EQ(10.0f, root_child0->layout().computedY);
}
