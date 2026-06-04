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

// Regression: a Row with a fixed-width sibling and a flex-grow Block whose
// content used width:100%. The Block grows to fill the remaining space, but the
// flex-basis phase measures AUTO-size items against NaN and collapses their
// subtree to 0. Without the post-resolution relayout, the grown Block's content
// stayed 0-wide (the "only the sidebar renders" bug in examples/opengl_demo).
TEST(FlexTests, flex_grow_block_relayouts_collapsed_subtree) {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<Dimensions>().width = 1024.0f;
    root->style().modify<Dimensions>().height = 768.0f;

    // Fixed-width sidebar (no grow) — already rendered correctly before the fix.
    auto sidebar = std::make_shared<Node>();
    sidebar->style().modify<Dimensions>().width = 220.0f;
    sidebar->style().modify<Dimensions>().height = CSSValue(100.0f, CSSUnit::PERCENT);
    root->addChild(sidebar);

    // Block content area that grows to fill the rest (the "Expanded" wrapper).
    auto content = std::make_shared<Node>(); // default OuterDisplay::Block
    content->style().modify<CSSFlex>().flexGrow = 1.0f;
    root->addChild(content);

    // Content sized to 100% of the (grown) Block; this is what collapsed to 0.
    auto inner = std::make_shared<Node>();
    inner->style().modify<Dimensions>().width = CSSValue(100.0f, CSSUnit::PERCENT);
    inner->style().modify<Dimensions>().height = CSSValue(100.0f, CSSUnit::PERCENT);
    content->addChild(inner);

    // A grandchild so `inner` is not a leaf and the relayout must descend.
    auto innerChild = std::make_shared<Node>();
    innerChild->style().modify<Dimensions>().width = CSSValue(100.0f, CSSUnit::PERCENT);
    innerChild->style().modify<Dimensions>().height = 40.0f;
    inner->addChild(innerChild);

    root->calculate(1024, 768);

    // The Block grows to 1024 - 220 = 804 and stretches to full height.
    ASSERT_FLOAT_EQ(804.0f, content->layout().computedWidth);
    ASSERT_FLOAT_EQ(768.0f, content->layout().computedHeight);
    ASSERT_FLOAT_EQ(220.0f, content->layout().computedX);

    // The fix: the grown Block's subtree is re-laid-out at the definite size
    // instead of keeping the width-0 layout from the flex-basis (NaN) phase.
    ASSERT_FLOAT_EQ(804.0f, inner->layout().computedWidth);
    ASSERT_FLOAT_EQ(768.0f, inner->layout().computedHeight);
    ASSERT_FLOAT_EQ(220.0f, inner->layout().computedX);
    ASSERT_FLOAT_EQ(804.0f, innerChild->layout().computedWidth);
    ASSERT_FLOAT_EQ(220.0f, innerChild->layout().computedX);
}

// Multi-frame stability: the engine recomputes layout every frame (syncLayoutNode
// re-dirties all nodes, then calculate() runs layoutImpl + startUpdatingPositions).
// startUpdatingPositions does computedX += parentX, so if a relaid-out subtree is
// not reset to RELATIVE coords each frame, positions drift right every frame.
static void dirtyAll(const std::shared_ptr<Node> &n) {
    n->style().dirty = true;
    for (auto &c: n->children) dirtyAll(c);
}

TEST(FlexTests, flex_grow_block_relayout_is_frame_stable) {
    // Mirrors the demo's nesting: Row[ sidebar, Expanded(Block) ] where the
    // Expanded holds a flex Column, which holds a flex Row, which holds another
    // Expanded(Block) — i.e. the recursive grow/relayout path.
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Row;
    root->style().modify<Dimensions>().width = 1024.0f;
    root->style().modify<Dimensions>().height = 768.0f;

    auto sidebar = std::make_shared<Node>();
    sidebar->style().modify<Dimensions>().width = 220.0f;
    sidebar->style().modify<Dimensions>().height = CSSValue(100.0f, CSSUnit::PERCENT);
    root->addChild(sidebar);

    auto content = std::make_shared<Node>();                 // Expanded (Block, grow)
    content->style().modify<CSSFlex>().flexGrow = 1.0f;
    root->addChild(content);

    auto col = std::make_shared<Node>();                     // Column (flex)
    col->setDisplay(OuterDisplay::Flex);
    col->style().modify<CSSFlex>().direction = FlexDirection::Column;
    col->style().modify<Dimensions>().width = CSSValue(100.0f, CSSUnit::PERCENT);
    col->style().modify<Dimensions>().height = CSSValue(100.0f, CSSUnit::PERCENT);
    content->addChild(col);

    auto row = std::make_shared<Node>();                     // inner Row (flex)
    row->setDisplay(OuterDisplay::Flex);
    row->style().modify<CSSFlex>().direction = FlexDirection::Row;
    row->style().modify<Dimensions>().width = CSSValue(100.0f, CSSUnit::PERCENT);
    row->style().modify<Dimensions>().height = 104.0f;
    col->addChild(row);

    auto tile = std::make_shared<Node>();                    // nested Expanded (Block, grow)
    tile->style().modify<CSSFlex>().flexGrow = 1.0f;
    row->addChild(tile);

    auto leaf = std::make_shared<Node>();
    leaf->style().modify<Dimensions>().width = CSSValue(100.0f, CSSUnit::PERCENT);
    leaf->style().modify<Dimensions>().height = 40.0f;
    tile->addChild(leaf);

    // Three "frames" — re-dirty everything between them like the real UISystem does.
    for (int frame = 0; frame < 3; ++frame) {
        dirtyAll(root);
        root->calculate(1024, 768);

        EXPECT_FLOAT_EQ(220.0f, content->layout().computedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(220.0f, col->layout().computedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(220.0f, row->layout().computedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(220.0f, tile->layout().computedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(220.0f, leaf->layout().computedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(804.0f, tile->layout().computedWidth) << "frame " << frame;
    }
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
