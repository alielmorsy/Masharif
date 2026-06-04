#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"
#include <iostream>
#include <iomanip>

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

// Full reproduction of the examples/opengl_demo dashboard skeleton (bad.png):
// the content area overflows the window on BOTH axes. This asserts that every
// box in the deep, padded, gap-spaced, multi-grow nesting stays inside the
// 1024x768 surface. If this fails, the overflow is an engine bug and the
// failing numbers point at which level loses the constraint.
namespace {
using NodePtr = std::shared_ptr<Node>;

NodePtr block() { return std::make_shared<Node>(); }                 // OuterDisplay::Block
NodePtr flexRow() {
    auto n = std::make_shared<Node>();
    n->setDisplay(OuterDisplay::Flex);
    n->style().modify<CSSFlex>().direction = FlexDirection::Row;
    return n;
}
NodePtr flexCol() {
    auto n = std::make_shared<Node>();
    n->setDisplay(OuterDisplay::Flex);
    n->style().modify<CSSFlex>().direction = FlexDirection::Column;
    return n;
}
void setSize(const NodePtr &n, CSSValue w, CSSValue h) {
    n->style().modify<Dimensions>().width = w;
    n->style().modify<Dimensions>().height = h;
}
void setGrow(const NodePtr &n, float g) { n->style().modify<CSSFlex>().flexGrow = g; }
void setPadding(const NodePtr &n, float all) {
    auto &p = n->style().modify<PaddingEdge>();
    p.top = p.right = p.bottom = p.left = CSSValue(all);
}
const CSSValue pct100{100.0f, CSSUnit::PERCENT};
} // namespace

TEST(FlexTests, dashboard_skeleton_stays_within_surface) {
    // root Box (Block, fills surface)
    auto root = block();
    setSize(root, pct100, pct100);

    // Row[ sidebar(fixed 220), content(Expanded) ], cross = stretch
    auto row = flexRow();
    setSize(row, pct100, pct100);
    root->addChild(row);

    auto sidebar = block();
    setSize(sidebar, CSSValue(220.0f), pct100);
    row->addChild(sidebar);

    auto content = block();                 // Expanded
    setGrow(content, 1.0f);
    row->addChild(content);

    // content > Column(100%/100%) > [ header(h=68), area(Expanded) ]
    auto ccol = flexCol();
    setSize(ccol, pct100, pct100);
    content->addChild(ccol);

    auto header = block();
    setSize(header, pct100, CSSValue(68.0f));
    ccol->addChild(header);

    auto area = block();                    // Expanded
    setGrow(area, 1.0f);
    ccol->addChild(area);

    // area > Column(100%/100%, padding 28, gap 28) > [ statRow(h=104), lower(Expanded) ]
    auto acol = flexCol();
    setSize(acol, pct100, pct100);
    setPadding(acol, 28.0f);
    acol->style().modify<CSSFlex>().gap.row = CSSValue(28.0f);
    area->addChild(acol);

    auto statRow = flexRow();
    setSize(statRow, pct100, CSSValue(104.0f));
    statRow->style().modify<CSSFlex>().gap.column = CSSValue(28.0f);
    acol->addChild(statRow);

    NodePtr tiles[3];
    for (auto &t : tiles) { t = block(); setGrow(t, 1.0f); statRow->addChild(t); }

    auto lower = block();                   // Expanded
    setGrow(lower, 1.0f);
    acol->addChild(lower);

    // lower > Row(100%/100%, gap 28) > [ chart(grow 62), activity(grow 38) ]
    auto lrow = flexRow();
    setSize(lrow, pct100, pct100);
    lrow->style().modify<CSSFlex>().gap.column = CSSValue(28.0f);
    lower->addChild(lrow);

    auto chart = block();    setGrow(chart, 62.0f);    lrow->addChild(chart);
    auto activity = block(); setGrow(activity, 38.0f);  lrow->addChild(activity);

    root->calculate(1024, 768);

    auto rightEdge  = [](const NodePtr &n) { return n->layout().computedX + n->layout().computedWidth; };
    auto bottomEdge = [](const NodePtr &n) { return n->layout().computedY + n->layout().computedHeight; };

    // Content area: 1024 - 220 = 804, anchored right after the sidebar.
    EXPECT_FLOAT_EQ(804.0f, content->layout().computedWidth);
    EXPECT_FLOAT_EQ(220.0f, content->layout().computedX);
    EXPECT_FLOAT_EQ(804.0f, ccol->layout().computedWidth);
    EXPECT_FLOAT_EQ(804.0f, area->layout().computedWidth);

    // Nothing may cross the right or bottom edge of the 1024x768 surface.
    for (auto &n : {content, ccol, header, area, acol, statRow, lower, lrow,
                    tiles[0], tiles[1], tiles[2], chart, activity}) {
        EXPECT_LE(rightEdge(n),  1024.0f + 0.5f) << "node overflows right edge";
        EXPECT_LE(bottomEdge(n), 768.0f + 0.5f)  << "node overflows bottom edge";
    }

    // The three stat tiles share the padded row evenly: (804 - 56 - 56) / 3.
    EXPECT_NEAR((804.0f - 56.0f - 56.0f) / 3.0f, tiles[0]->layout().computedWidth, 0.5f);
    EXPECT_LE(rightEdge(tiles[2]), 996.0f + 0.5f);          // inside the 28px content padding
    EXPECT_LE(rightEdge(activity), 996.0f + 0.5f);
    EXPECT_LE(bottomEdge(chart),   740.0f + 0.5f);          // bars stay above the bottom padding
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
