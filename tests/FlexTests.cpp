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

// Regression for the partial-solve cache bug (examples/opengl_demo sidebar collapse, bad.png):
// when only a SIBLING (content) subtree is dirtied and re-solved, the CLEAN sidebar's
// AUTO-main + flex-grow inner column was left collapsed. The common-ancestor Row re-solves and
// re-measures the clean sidebar via layoutImpl(..., NaN) in its flex-basis phase — the sidebar's
// _lastAvailH held a definite value, so that memo MISSES and the AUTO main axis shrink-collapses.
// Then the sidebar's layoutContentsWithDefiniteSize takes its _lastDef cache HIT (the NaN collapse
// is invisible to a gate keyed only on _lastDef) and skips re-laying the now-collapsed children.
// The fix re-expands a subtree whose strategy was re-run against a NaN main axis since the last
// definite pass. Mirrors the bridge: every container Flex, Expand => flexGrow + flex-basis:0px.
TEST(FlexTests, partial_relayout_keeps_clean_autogrow_sidebar_expanded) {
    const CSSValue autoSize{};                       // AUTO (NaN default -> unit AUTO)
    auto px = [](float v) { return CSSValue(v, CSSUnit::PX); };
    auto setBorder = [&](const NodePtr &n, float w) {
        auto &b = n->style().modify<BorderProperties>();
        b.widthTop = b.widthBottom = b.widthLeft = b.widthRight = px(w);
    };
    auto setGap = [&](const NodePtr &n, float g) {
        auto &f = n->style().modify<CSSFlex>();
        f.gap.row = px(g);
        f.gap.column = px(g);
    };
    auto setStretch = [](const NodePtr &n) { n->style().modify<CSSFlex>().alignItems = AlignItems::Stretch; };
    // Expand => flexGrow + flex-basis:0px, exactly like LayoutBridge::ApplyStyle.
    auto setExpandGrow = [&](const NodePtr &n, float g) {
        auto &f = n->style().modify<CSSFlex>();
        f.flexGrow = g;
        f.flexBasis = px(0.0f);
    };

    const float W = 1024.0f, H = 768.0f, border = 1.0f, pad = 12.0f, gap = 6.0f;

    // root: Flex Column pinned to the surface (the bridge pins width/height to PX).
    auto root = flexCol();
    setSize(root, px(W), px(H));

    // Row: Flex Row, cross=Stretch, height=Expand (AUTO main axis + grow under a column root).
    auto row = flexRow();
    setSize(row, autoSize, autoSize);
    setExpandGrow(row, 1.0f);
    setStretch(row);
    root->addChild(row);

    // Sidebar: Flex Column, width=220px, height AUTO (Wrap), 1px border, child align=Stretch.
    auto sidebar = flexCol();
    setSize(sidebar, px(220.0f), autoSize);
    setBorder(sidebar, border);
    setStretch(sidebar);
    row->addChild(sidebar);

    // innerCol: Flex Column, height=Expand (AUTO main + grow + flex-basis 0px), padding 12, gap 6.
    auto innerCol = flexCol();
    setSize(innerCol, autoSize, autoSize);
    setExpandGrow(innerCol, 1.0f);
    setPadding(innerCol, pad);
    setGap(innerCol, gap);
    setStretch(innerCol);
    sidebar->addChild(innerCol);

    auto logo = flexCol();                           // fixed-height block (logo mark)
    setSize(logo, autoSize, px(52.0f));
    innerCol->addChild(logo);

    auto navCol = flexCol();                         // column of fixed-height nav rows
    setSize(navCol, autoSize, autoSize);
    setGap(navCol, gap);
    innerCol->addChild(navCol);
    NodePtr navRows[3];
    for (auto &r : navRows) {
        r = flexCol();
        setSize(r, autoSize, px(40.0f));
        navCol->addChild(r);
    }

    auto spacer = flexCol();                         // flex-grow spacer pushes footer to bottom
    setSize(spacer, autoSize, autoSize);
    setGrow(spacer, 1.0f);
    innerCol->addChild(spacer);

    auto footer = flexCol();                         // fixed-height footer (user card)
    setSize(footer, autoSize, px(56.0f));
    innerCol->addChild(footer);

    // Content: the other Row child, Flex Column, width=Expand (grow). Holds a child we rebuild.
    auto content = flexCol();
    setSize(content, autoSize, autoSize);
    setExpandGrow(content, 1.0f);
    row->addChild(content);

    auto contentChild = flexCol();
    setSize(contentChild, pct100, px(120.0f));
    content->addChild(contentChild);

    const float tol = 1.0f;

    // ── Initial solve ─────────────────────────────────────────────────────────
    root->calculate(W, H);

    // Sanity: the sidebar is genuinely expanded (not a degenerate all-zero baseline that a
    // collapse would trivially "preserve"). The inner column nearly fills the surface, the
    // flex-grow spacer ate most of it, and the footer sits near the bottom.
    EXPECT_GT(innerCol->layout().computedHeight, 700.0f) << "inner column not expanded initially";
    EXPECT_GT(spacer->layout().computedHeight, 100.0f)   << "spacer not grown initially";
    EXPECT_GT(footer->layout().computedY, 600.0f)        << "footer not bottom-anchored initially";
    EXPECT_GT(navRows[0]->layout().computedHeight, 20.0f) << "nav row collapsed initially";

    // Capture the correct baseline; a sibling-only relayout must leave the clean sidebar
    // pixel-identical (avoids hard-coding engine specifics like exact nav-row height).
    const float innerH0  = innerCol->layout().computedHeight;
    const float spacerH0 = spacer->layout().computedHeight;
    const float footerY0 = footer->layout().computedY;
    const float navH0    = navRows[0]->layout().computedHeight;
    const float navGap0  = navRows[1]->layout().computedY - navRows[0]->layout().computedY;

    // ── Partial relayout: dirty ONLY the content subtree (simulate the chart Each rebuild) ──
    auto newContentChild = flexCol();
    setSize(newContentChild, pct100, px(140.0f));
    content->children.clear();
    content->addChild(newContentChild);              // addChild sets content dirty
    content->markDirtyToRoot();                      // propagate _descendantDirty up through row to root

    root->calculate(W, H);

    // The clean sidebar subtree must be unchanged by a sibling-only relayout (the bug left it
    // collapsed: inner column shrunk, spacer/footer gone, nav rows overlapping at height 0).
    EXPECT_NEAR(innerH0,  innerCol->layout().computedHeight, tol)
        << "inner column collapsed after sibling-only relayout";
    EXPECT_NEAR(spacerH0, spacer->layout().computedHeight, tol)
        << "flex-grow spacer collapsed after sibling-only relayout";
    EXPECT_NEAR(footerY0, footer->layout().computedY, tol)
        << "footer no longer bottom-anchored after sibling-only relayout";
    EXPECT_NEAR(navH0, navRows[0]->layout().computedHeight, tol)
        << "nav row lost its height after sibling-only relayout";
    EXPECT_NEAR(navGap0, navRows[1]->layout().computedY - navRows[0]->layout().computedY, tol)
        << "nav rows overlap (lost gap) after sibling-only relayout";
}

// Horizontal analog of the sidebar collapse — the header bug in bad.png. The header Row holds
// a flex-grow Spacer that pushes a fixed search box + button to the RIGHT. Crucially the header
// Row's WIDTH comes from CROSS-axis stretch (it is the cross child of the Header column), not
// main-axis grow — a different path than the sidebar's vertical spacer. A sibling-only relayout
// (the content area) must leave the clean header pinned right.
TEST(FlexTests, partial_relayout_keeps_clean_header_spacer_pushed_right) {
    const CSSValue autoSize{};
    auto px = [](float v) { return CSSValue(v, CSSUnit::PX); };
    auto setBorder = [&](const NodePtr &n, float w) {
        auto &b = n->style().modify<BorderProperties>();
        b.widthTop = b.widthBottom = b.widthLeft = b.widthRight = px(w);
    };
    auto setGap = [&](const NodePtr &n, float g) {
        auto &f = n->style().modify<CSSFlex>();
        f.gap.row = px(g);
        f.gap.column = px(g);
    };
    auto setAlign = [](const NodePtr &n, AlignItems a) { n->style().modify<CSSFlex>().alignItems = a; };
    auto setExpandGrow = [&](const NodePtr &n, float g) {
        auto &f = n->style().modify<CSSFlex>();
        f.flexGrow = g;
        f.flexBasis = px(0.0f);
    };
    auto setPaddingLR = [&](const NodePtr &n, float lr) {
        auto &p = n->style().modify<PaddingEdge>();
        p.left = p.right = px(lr);
    };

    const float W = 1024.0f, H = 768.0f;

    // root(col, pinned) > appRow(row, grow-h, stretch) > [ sidebar(220), content(grow-w) ]
    auto root = flexCol();
    setSize(root, px(W), px(H));

    auto appRow = flexRow();
    setSize(appRow, autoSize, autoSize);
    setExpandGrow(appRow, 1.0f);
    setAlign(appRow, AlignItems::Stretch);
    root->addChild(appRow);

    auto sidebar = flexCol();                        // fixed 220 -> content area is 804 wide
    setSize(sidebar, px(220.0f), autoSize);
    appRow->addChild(sidebar);
    auto sbChild = flexCol();
    setSize(sbChild, autoSize, px(100.0f));
    sidebar->addChild(sbChild);

    auto content = flexCol();                        // grows to fill the remaining width
    setSize(content, autoSize, autoSize);
    setExpandGrow(content, 1.0f);
    setAlign(content, AlignItems::Stretch);
    appRow->addChild(content);

    // header: Box -> column, fixed 68 height, AUTO (cross-stretched) width, 1px border.
    auto header = flexCol();
    setSize(header, autoSize, px(68.0f));
    setBorder(header, 1.0f);
    setAlign(header, AlignItems::Stretch);
    content->addChild(header);

    // headerRow: fills the 68px height (grow), width comes from Header's cross-stretch.
    auto headerRow = flexRow();
    setSize(headerRow, autoSize, autoSize);
    setExpandGrow(headerRow, 1.0f);
    setGap(headerRow, 16.0f);
    setPaddingLR(headerRow, 28.0f);
    setAlign(headerRow, AlignItems::FlexCenter);
    header->addChild(headerRow);

    auto title = flexCol();                          // AUTO width, shrink-to-fit to its child
    setSize(title, autoSize, autoSize);
    headerRow->addChild(title);
    auto titleChild = flexCol();
    setSize(titleChild, px(120.0f), px(22.0f));
    title->addChild(titleChild);

    auto spacer = flexRow();                         // flex-grow spacer pushes the rest right
    setSize(spacer, autoSize, autoSize);
    setExpandGrow(spacer, 1.0f);
    headerRow->addChild(spacer);

    auto search = flexRow();                         // fixed 220 search box
    setSize(search, px(220.0f), px(36.0f));
    headerRow->addChild(search);

    auto button = flexCol();                         // AUTO width button
    setSize(button, autoSize, autoSize);
    headerRow->addChild(button);
    auto buttonChild = flexCol();
    setSize(buttonChild, px(70.0f), px(34.0f));
    button->addChild(buttonChild);

    // contentArea: the sibling we dirty (stands in for the animating chart).
    auto contentArea = flexCol();
    setSize(contentArea, autoSize, autoSize);
    setExpandGrow(contentArea, 1.0f);
    setAlign(contentArea, AlignItems::Stretch);
    content->addChild(contentArea);
    auto areaChild = flexCol();
    setSize(areaChild, autoSize, px(120.0f));
    contentArea->addChild(areaChild);

    // ── Initial solve ─────────────────────────────────────────────────────────
    root->calculate(W, H);

    // content area spans x∈[220,1024]; header content padding 28 + border 1, so the search box's
    // right edge should be near ~995 and the spacer should have eaten most of the row.
    EXPECT_GT(spacer->layout().computedWidth, 150.0f) << "spacer not grown initially";
    EXPECT_GT(search->layout().computedX + search->layout().computedWidth, 700.0f)
        << "search box not pushed right initially";

    const float tol = 1.0f;
    const float spacerW0 = spacer->layout().computedWidth;
    const float searchX0  = search->layout().computedX;
    const float buttonX0  = button->layout().computedX;

    auto dump = [&](const char* phase) {
        std::cout << "[" << phase << "] content.w=" << content->layout().computedWidth
                  << " header.w=" << header->layout().computedWidth
                  << " headerRow.w=" << headerRow->layout().computedWidth
                  << " headerRow.h=" << headerRow->layout().computedHeight
                  << " spacer.w=" << spacer->layout().computedWidth
                  << " title.w=" << title->layout().computedWidth
                  << " search.w=" << search->layout().computedWidth
                  << " search.x=" << search->layout().computedX << "\n";
    };
    dump("initial");

    // ── Partial relayout: dirty ONLY the content-area subtree ──
    auto newAreaChild = flexCol();
    setSize(newAreaChild, autoSize, px(140.0f));
    contentArea->children.clear();
    contentArea->addChild(newAreaChild);
    contentArea->markDirtyToRoot();

    root->calculate(W, H);
    dump("relayout");

    // The clean header must be unchanged: spacer still wide, search/button still pinned right.
    EXPECT_NEAR(spacerW0, spacer->layout().computedWidth, tol)
        << "header spacer collapsed after sibling-only relayout";
    EXPECT_NEAR(searchX0, search->layout().computedX, tol)
        << "search box shifted left after sibling-only relayout";
    EXPECT_NEAR(buttonX0, button->layout().computedX, tol)
        << "button shifted left after sibling-only relayout";
}

// Third facet of the relayout bug: a CENTERED element drifts (the sidebar logo's accent box
// "moves to the right"). The logo Row is centered horizontally inside the logo Box
// (align-items: center); the accent box is the Row's first child. A sibling-only relayout must
// not move it — its X is driven by the row's width and the box's content width via the
// centering math, so a stale size there shifts the whole row.
TEST(FlexTests, partial_relayout_keeps_clean_centered_logo_in_place) {
    const CSSValue autoSize{};
    auto px = [](float v) { return CSSValue(v, CSSUnit::PX); };
    auto setGap = [&](const NodePtr &n, float g) {
        auto &f = n->style().modify<CSSFlex>();
        f.gap.row = px(g);
        f.gap.column = px(g);
    };
    auto setAlign = [](const NodePtr &n, AlignItems a) { n->style().modify<CSSFlex>().alignItems = a; };
    auto setJustify = [](const NodePtr &n, JustifyContent j) { n->style().modify<CSSFlex>().justifyContent = j; };
    auto setExpandGrow = [&](const NodePtr &n, float g) {
        auto &f = n->style().modify<CSSFlex>();
        f.flexGrow = g;
        f.flexBasis = px(0.0f);
    };
    auto setPadding4 = [&](const NodePtr &n, float t, float r, float b, float l) {
        auto &p = n->style().modify<PaddingEdge>();
        p.top = px(t); p.right = px(r); p.bottom = px(b); p.left = px(l);
    };
    auto setPaddingAll = [&](const NodePtr &n, float a) {
        auto &p = n->style().modify<PaddingEdge>();
        p.top = p.right = p.bottom = p.left = px(a);
    };
    auto setBorder = [&](const NodePtr &n, float w) {
        auto &b = n->style().modify<BorderProperties>();
        b.widthTop = b.widthBottom = b.widthLeft = b.widthRight = px(w);
    };
    auto setCenterChild = [&](const NodePtr &n) {
        setJustify(n, JustifyContent::FlexCenter);
        setAlign(n, AlignItems::FlexCenter);
    };

    const float W = 1024.0f, H = 768.0f;

    auto root = flexCol();
    setSize(root, px(W), px(H));

    auto appRow = flexRow();
    setSize(appRow, autoSize, autoSize);
    setExpandGrow(appRow, 1.0f);
    setAlign(appRow, AlignItems::Stretch);
    root->addChild(appRow);

    // sidebar Box: fixed 220 wide, 1px border, stretches its child column.
    auto sidebar = flexCol();
    setSize(sidebar, px(220.0f), autoSize);
    setBorder(sidebar, 1.0f);
    setAlign(sidebar, AlignItems::Stretch);
    appRow->addChild(sidebar);

    // inner column: Expand height (grow), cross=stretch, padding 12, gap 6 — exactly the
    // wrapper that holds the demo logo, so the logo sits two stretch levels deep.
    auto innerCol = flexCol();
    setSize(innerCol, autoSize, autoSize);
    setExpandGrow(innerCol, 1.0f);
    setPaddingAll(innerCol, 12.0f);
    setGap(innerCol, 6.0f);
    setAlign(innerCol, AlignItems::Stretch);
    sidebar->addChild(innerCol);

    // logo Box: column, AUTO size (cross-stretched), padding {14,12,14,12}, centered both ways.
    auto logoBox = flexCol();
    setSize(logoBox, autoSize, autoSize);
    setPadding4(logoBox, 14.0f, 12.0f, 14.0f, 12.0f);
    setCenterChild(logoBox);
    innerCol->addChild(logoBox);

    auto logoRow = flexRow();                        // shrink-to-fit row, centered in logoBox
    setSize(logoRow, autoSize, autoSize);
    setGap(logoRow, 12.0f);
    setAlign(logoRow, AlignItems::FlexCenter);
    logoBox->addChild(logoRow);

    auto accent = flexCol();                         // the box that "moves right"; 28x28, centers a dot
    setSize(accent, px(28.0f), px(28.0f));
    setCenterChild(accent);
    logoRow->addChild(accent);
    auto dot = flexCol();
    setSize(dot, px(14.0f), px(14.0f));
    accent->addChild(dot);
    auto label = flexCol();                          // stands in for the "Lumora" text
    setSize(label, px(90.0f), px(20.0f));
    logoRow->addChild(label);

    auto navFiller = flexCol();                      // fills the rest of the inner column height
    setSize(navFiller, autoSize, autoSize);
    setExpandGrow(navFiller, 1.0f);
    innerCol->addChild(navFiller);

    auto content = flexCol();
    setSize(content, autoSize, autoSize);
    setExpandGrow(content, 1.0f);
    setAlign(content, AlignItems::Stretch);
    appRow->addChild(content);

    auto contentArea = flexCol();
    setSize(contentArea, autoSize, autoSize);
    setExpandGrow(contentArea, 1.0f);
    setAlign(contentArea, AlignItems::Stretch);
    content->addChild(contentArea);
    auto areaChild = flexCol();
    setSize(areaChild, autoSize, px(120.0f));
    contentArea->addChild(areaChild);

    auto dump = [&](const char* phase) {
        std::cout << "[" << phase << "] logoBox.w=" << logoBox->layout().computedWidth
                  << " logoRow.w=" << logoRow->layout().computedWidth
                  << " logoRow.x=" << logoRow->layout().computedX
                  << " accent.x=" << accent->layout().computedX
                  << " accent.w=" << accent->layout().computedWidth << "\n";
    };

    // ── Initial solve ─────────────────────────────────────────────────────────
    root->calculate(W, H);
    dump("initial");

    const float tol = 1.0f;
    const float accentX0  = accent->layout().computedX;
    const float logoRowX0 = logoRow->layout().computedX;
    const float logoRowW0 = logoRow->layout().computedWidth;

    EXPECT_GT(accentX0, 12.0f) << "logo not centered initially (should be inset past the padding)";

    // ── Partial relayout: dirty ONLY the content-area subtree ──
    auto newAreaChild = flexCol();
    setSize(newAreaChild, autoSize, px(140.0f));
    contentArea->children.clear();
    contentArea->addChild(newAreaChild);
    contentArea->markDirtyToRoot();

    root->calculate(W, H);
    dump("relayout");

    EXPECT_NEAR(accentX0, accent->layout().computedX, tol)
        << "accent box drifted horizontally after sibling-only relayout";
    EXPECT_NEAR(logoRowX0, logoRow->layout().computedX, tol)
        << "logo row drifted after sibling-only relayout";
    EXPECT_NEAR(logoRowW0, logoRow->layout().computedWidth, tol)
        << "logo row width changed after sibling-only relayout";
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
