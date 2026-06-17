#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"
#include <iostream>
#include <iomanip>

using namespace masharif;

TEST(FlexTests, flex_basis_flex_grow_column) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root_child0->GetStyle().Modify<CSSFlex>().FlexBasis = 50.0f;
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root->AddChild(root_child1);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(100.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(100.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(75.0f, root_child0->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedY);

    ASSERT_FLOAT_EQ(100.0f, root_child1->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(25.0f, root_child1->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(75.0f, root_child1->GetLayout().ComputedY);
}

TEST(FlexTests, flex_shrink_flex_grow_row) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root->GetStyle().Modify<Dimensions>().Height = 500.0f;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<CSSFlex>().FlexShrink = 1.0f;
    root_child0->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<CSSFlex>().FlexShrink = 1.0f;
    root_child1->GetStyle().Modify<Dimensions>().Width = 500.0f;
    root_child1->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(root_child1);

    root->Calculate(500, 500);

    ASSERT_FLOAT_EQ(500.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(500.0f, root->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(250.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child0->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(0.0f, root_child0->GetLayout().ComputedX);

    ASSERT_FLOAT_EQ(250.0f, root_child1->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(100.0f, root_child1->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(250.0f, root_child1->GetLayout().ComputedX);
}

TEST(FlexTests, flex_basis_overrides_main_size) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;

    auto root_child0 = std::make_shared<Node>();
    root_child0->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root_child0->GetStyle().Modify<CSSFlex>().FlexBasis = 50.0f;
    root_child0->GetStyle().Modify<Dimensions>().Height = 20.0f;
    root->AddChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root_child1->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child1);

    auto root_child2 = std::make_shared<Node>();
    root_child2->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root_child2->GetStyle().Modify<Dimensions>().Height = 10.0f;
    root->AddChild(root_child2);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(100.0f, root_child0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(60.0f, root_child0->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(100.0f, root_child1->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(20.0f, root_child1->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(60.0f, root_child1->GetLayout().ComputedY);

    ASSERT_FLOAT_EQ(100.0f, root_child2->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(20.0f, root_child2->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(80.0f, root_child2->GetLayout().ComputedY);
}

TEST(FlexTests, flex_direction_column_reverse) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::ColumnReverse;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 20.0f;
    root->AddChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->GetStyle().Modify<Dimensions>().Width = 100.0f;
    child1->GetStyle().Modify<Dimensions>().Height = 20.0f;
    root->AddChild(child1);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(80.0f, child0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(60.0f, child1->GetLayout().ComputedY);
}

// Regression: a Row with a fixed-width sibling and a flex-grow Block whose
// content used width:100%. The Block grows to fill the remaining space, but the
// flex-basis phase measures AUTO-size items against NaN and collapses their
// subtree to 0. Without the post-resolution relayout, the grown Block's content
// stayed 0-wide (the "only the sidebar renders" bug in examples/opengl_demo).
TEST(FlexTests, flex_grow_block_relayouts_collapsed_subtree) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 1024.0f;
    root->GetStyle().Modify<Dimensions>().Height = 768.0f;

    // Fixed-width sidebar (no grow) — already rendered correctly before the fix.
    auto sidebar = std::make_shared<Node>();
    sidebar->GetStyle().Modify<Dimensions>().Width = 220.0f;
    sidebar->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f, CSSUnit::Percent);
    root->AddChild(sidebar);

    // Block content area that grows to fill the rest (the "Expanded" wrapper).
    auto content = std::make_shared<Node>(); // default OuterDisplay::Block
    content->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root->AddChild(content);

    // Content sized to 100% of the (grown) Block; this is what collapsed to 0.
    auto inner = std::make_shared<Node>();
    inner->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f, CSSUnit::Percent);
    inner->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f, CSSUnit::Percent);
    content->AddChild(inner);

    // A grandchild so `inner` is not a leaf and the relayout must descend.
    auto innerChild = std::make_shared<Node>();
    innerChild->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f, CSSUnit::Percent);
    innerChild->GetStyle().Modify<Dimensions>().Height = 40.0f;
    inner->AddChild(innerChild);

    root->Calculate(1024, 768);

    // The Block grows to 1024 - 220 = 804 and stretches to full height.
    ASSERT_FLOAT_EQ(804.0f, content->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(768.0f, content->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(220.0f, content->GetLayout().ComputedX);

    // The fix: the grown Block's subtree is re-laid-out at the definite size
    // instead of keeping the width-0 layout from the flex-basis (NaN) phase.
    ASSERT_FLOAT_EQ(804.0f, inner->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(768.0f, inner->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(220.0f, inner->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(804.0f, innerChild->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(220.0f, innerChild->GetLayout().ComputedX);
}

// Multi-frame stability: the engine recomputes layout every frame (syncLayoutNode
// re-dirties all nodes, then Calculate() runs layoutImpl + startUpdatingPositions).
// startUpdatingPositions does computedX += parentX, so if a relaid-out subtree is
// not reset to RELATIVE coords each frame, positions drift right every frame.
static void dirtyAll(const std::shared_ptr<Node> &n) {
    n->GetStyle().Dirty = true;
    for (auto &c: n->Children()) dirtyAll(c);
}

TEST(FlexTests, flex_grow_block_relayout_is_frame_stable) {
    // Mirrors the demo's nesting: Row[ sidebar, Expanded(Block) ] where the
    // Expanded holds a flex Column, which holds a flex Row, which holds another
    // Expanded(Block) — i.e. the recursive grow/relayout path.
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = 1024.0f;
    root->GetStyle().Modify<Dimensions>().Height = 768.0f;

    auto sidebar = std::make_shared<Node>();
    sidebar->GetStyle().Modify<Dimensions>().Width = 220.0f;
    sidebar->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f, CSSUnit::Percent);
    root->AddChild(sidebar);

    auto content = std::make_shared<Node>();                 // Expanded (Block, grow)
    content->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root->AddChild(content);

    auto col = std::make_shared<Node>();                     // Column (flex)
    col->SetDisplay(OuterDisplay::Flex);
    col->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    col->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f, CSSUnit::Percent);
    col->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f, CSSUnit::Percent);
    content->AddChild(col);

    auto row = std::make_shared<Node>();                     // inner Row (flex)
    row->SetDisplay(OuterDisplay::Flex);
    row->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    row->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f, CSSUnit::Percent);
    row->GetStyle().Modify<Dimensions>().Height = 104.0f;
    col->AddChild(row);

    auto tile = std::make_shared<Node>();                    // nested Expanded (Block, grow)
    tile->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    row->AddChild(tile);

    auto leaf = std::make_shared<Node>();
    leaf->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f, CSSUnit::Percent);
    leaf->GetStyle().Modify<Dimensions>().Height = 40.0f;
    tile->AddChild(leaf);

    // Three "frames" — re-dirty everything between them like the real UISystem does.
    for (int frame = 0; frame < 3; ++frame) {
        dirtyAll(root);
        root->Calculate(1024, 768);

        EXPECT_FLOAT_EQ(220.0f, content->GetLayout().ComputedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(220.0f, col->GetLayout().ComputedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(220.0f, row->GetLayout().ComputedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(220.0f, tile->GetLayout().ComputedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(220.0f, leaf->GetLayout().ComputedX) << "frame " << frame;
        EXPECT_FLOAT_EQ(804.0f, tile->GetLayout().ComputedWidth) << "frame " << frame;
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
    n->SetDisplay(OuterDisplay::Flex);
    n->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    return n;
}
NodePtr flexCol() {
    auto n = std::make_shared<Node>();
    n->SetDisplay(OuterDisplay::Flex);
    n->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    return n;
}
void setSize(const NodePtr &n, CSSValue w, CSSValue h) {
    n->GetStyle().Modify<Dimensions>().Width = w;
    n->GetStyle().Modify<Dimensions>().Height = h;
}
void setGrow(const NodePtr &n, float g) { n->GetStyle().Modify<CSSFlex>().FlexGrow = g; }
void setPadding(const NodePtr &n, float all) {
    auto &p = n->GetStyle().Modify<PaddingEdge>();
    p.Top = p.Right = p.Bottom = p.Left = CSSValue(all);
}
const CSSValue pct100{100.0f, CSSUnit::Percent};
} // namespace

TEST(FlexTests, dashboard_skeleton_stays_within_surface) {
    // root Box (Block, fills surface)
    auto root = block();
    setSize(root, pct100, pct100);

    // Row[ sidebar(fixed 220), content(Expanded) ], cross = stretch
    auto row = flexRow();
    setSize(row, pct100, pct100);
    root->AddChild(row);

    auto sidebar = block();
    setSize(sidebar, CSSValue(220.0f), pct100);
    row->AddChild(sidebar);

    auto content = block();                 // Expanded
    setGrow(content, 1.0f);
    row->AddChild(content);

    // content > Column(100%/100%) > [ header(h=68), area(Expanded) ]
    auto ccol = flexCol();
    setSize(ccol, pct100, pct100);
    content->AddChild(ccol);

    auto header = block();
    setSize(header, pct100, CSSValue(68.0f));
    ccol->AddChild(header);

    auto area = block();                    // Expanded
    setGrow(area, 1.0f);
    ccol->AddChild(area);

    // area > Column(100%/100%, padding 28, gap 28) > [ statRow(h=104), lower(Expanded) ]
    auto acol = flexCol();
    setSize(acol, pct100, pct100);
    setPadding(acol, 28.0f);
    acol->GetStyle().Modify<CSSFlex>().Gaps.Row = CSSValue(28.0f);
    area->AddChild(acol);

    auto statRow = flexRow();
    setSize(statRow, pct100, CSSValue(104.0f));
    statRow->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(28.0f);
    acol->AddChild(statRow);

    NodePtr tiles[3];
    for (auto &t : tiles) { t = block(); setGrow(t, 1.0f); statRow->AddChild(t); }

    auto lower = block();                   // Expanded
    setGrow(lower, 1.0f);
    acol->AddChild(lower);

    // lower > Row(100%/100%, gap 28) > [ chart(grow 62), activity(grow 38) ]
    auto lrow = flexRow();
    setSize(lrow, pct100, pct100);
    lrow->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(28.0f);
    lower->AddChild(lrow);

    auto chart = block();    setGrow(chart, 62.0f);    lrow->AddChild(chart);
    auto activity = block(); setGrow(activity, 38.0f);  lrow->AddChild(activity);

    root->Calculate(1024, 768);

    auto rightEdge  = [](const NodePtr &n) { return n->GetLayout().ComputedX + n->GetLayout().ComputedWidth; };
    auto bottomEdge = [](const NodePtr &n) { return n->GetLayout().ComputedY + n->GetLayout().ComputedHeight; };

    // Content area: 1024 - 220 = 804, anchored right after the sidebar.
    EXPECT_FLOAT_EQ(804.0f, content->GetLayout().ComputedWidth);
    EXPECT_FLOAT_EQ(220.0f, content->GetLayout().ComputedX);
    EXPECT_FLOAT_EQ(804.0f, ccol->GetLayout().ComputedWidth);
    EXPECT_FLOAT_EQ(804.0f, area->GetLayout().ComputedWidth);

    // Nothing may cross the right or bottom edge of the 1024x768 surface.
    for (auto &n : {content, ccol, header, area, acol, statRow, lower, lrow,
                    tiles[0], tiles[1], tiles[2], chart, activity}) {
        EXPECT_LE(rightEdge(n),  1024.0f + 0.5f) << "node overflows right edge";
        EXPECT_LE(bottomEdge(n), 768.0f + 0.5f)  << "node overflows bottom edge";
    }

    // The three stat tiles share the padded row evenly: (804 - 56 - 56) / 3.
    EXPECT_NEAR((804.0f - 56.0f - 56.0f) / 3.0f, tiles[0]->GetLayout().ComputedWidth, 0.5f);
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
    auto px = [](float v) { return CSSValue(v, CSSUnit::Px); };
    auto setBorder = [&](const NodePtr &n, float w) {
        auto &b = n->GetStyle().Modify<BorderProperties>();
        b.WidthTop = b.WidthBottom = b.WidthLeft = b.WidthRight = px(w);
    };
    auto setGap = [&](const NodePtr &n, float g) {
        auto &f = n->GetStyle().Modify<CSSFlex>();
        f.Gaps.Row = px(g);
        f.Gaps.Column = px(g);
    };
    auto setStretch = [](const NodePtr &n) { n->GetStyle().Modify<CSSFlex>().Align = AlignItems::Stretch; };
    // Expand => flexGrow + flex-basis:0px, exactly like LayoutBridge::ApplyStyle.
    auto setExpandGrow = [&](const NodePtr &n, float g) {
        auto &f = n->GetStyle().Modify<CSSFlex>();
        f.FlexGrow = g;
        f.FlexBasis = px(0.0f);
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
    root->AddChild(row);

    // Sidebar: Flex Column, width=220px, height AUTO (Wrap), 1px border, child align=Stretch.
    auto sidebar = flexCol();
    setSize(sidebar, px(220.0f), autoSize);
    setBorder(sidebar, border);
    setStretch(sidebar);
    row->AddChild(sidebar);

    // innerCol: Flex Column, height=Expand (AUTO main + grow + flex-basis 0px), padding 12, gap 6.
    auto innerCol = flexCol();
    setSize(innerCol, autoSize, autoSize);
    setExpandGrow(innerCol, 1.0f);
    setPadding(innerCol, pad);
    setGap(innerCol, gap);
    setStretch(innerCol);
    sidebar->AddChild(innerCol);

    auto logo = flexCol();                           // fixed-height block (logo mark)
    setSize(logo, autoSize, px(52.0f));
    innerCol->AddChild(logo);

    auto navCol = flexCol();                         // column of fixed-height nav rows
    setSize(navCol, autoSize, autoSize);
    setGap(navCol, gap);
    innerCol->AddChild(navCol);
    NodePtr navRows[3];
    for (auto &r : navRows) {
        r = flexCol();
        setSize(r, autoSize, px(40.0f));
        navCol->AddChild(r);
    }

    auto spacer = flexCol();                         // flex-grow spacer pushes footer to bottom
    setSize(spacer, autoSize, autoSize);
    setGrow(spacer, 1.0f);
    innerCol->AddChild(spacer);

    auto footer = flexCol();                         // fixed-height footer (user card)
    setSize(footer, autoSize, px(56.0f));
    innerCol->AddChild(footer);

    // Content: the other Row child, Flex Column, width=Expand (grow). Holds a child we rebuild.
    auto content = flexCol();
    setSize(content, autoSize, autoSize);
    setExpandGrow(content, 1.0f);
    row->AddChild(content);

    auto contentChild = flexCol();
    setSize(contentChild, pct100, px(120.0f));
    content->AddChild(contentChild);

    const float tol = 1.0f;

    // ── Initial solve ─────────────────────────────────────────────────────────
    root->Calculate(W, H);

    // Sanity: the sidebar is genuinely expanded (not a degenerate all-zero baseline that a
    // collapse would trivially "preserve"). The inner column nearly fills the surface, the
    // flex-grow spacer ate most of it, and the footer sits near the bottom.
    EXPECT_GT(innerCol->GetLayout().ComputedHeight, 700.0f) << "inner column not expanded initially";
    EXPECT_GT(spacer->GetLayout().ComputedHeight, 100.0f)   << "spacer not grown initially";
    EXPECT_GT(footer->GetLayout().ComputedY, 600.0f)        << "footer not bottom-anchored initially";
    EXPECT_GT(navRows[0]->GetLayout().ComputedHeight, 20.0f) << "nav row collapsed initially";

    // Capture the correct baseline; a sibling-only relayout must leave the clean sidebar
    // pixel-identical (avoids hard-coding engine specifics like exact nav-row height).
    const float innerH0  = innerCol->GetLayout().ComputedHeight;
    const float spacerH0 = spacer->GetLayout().ComputedHeight;
    const float footerY0 = footer->GetLayout().ComputedY;
    const float navH0    = navRows[0]->GetLayout().ComputedHeight;
    const float navGap0  = navRows[1]->GetLayout().ComputedY - navRows[0]->GetLayout().ComputedY;

    // ── Partial relayout: dirty ONLY the content subtree (simulate the chart Each rebuild) ──
    auto newContentChild = flexCol();
    setSize(newContentChild, pct100, px(140.0f));
    content->ClearChildren();
    content->AddChild(newContentChild);              // addChild sets content dirty
    content->MarkDirtyToRoot();                      // propagate _descendantDirty up through row to root

    root->Calculate(W, H);

    // The clean sidebar subtree must be unchanged by a sibling-only relayout (the bug left it
    // collapsed: inner column shrunk, spacer/footer gone, nav rows overlapping at height 0).
    EXPECT_NEAR(innerH0,  innerCol->GetLayout().ComputedHeight, tol)
        << "inner column collapsed after sibling-only relayout";
    EXPECT_NEAR(spacerH0, spacer->GetLayout().ComputedHeight, tol)
        << "flex-grow spacer collapsed after sibling-only relayout";
    EXPECT_NEAR(footerY0, footer->GetLayout().ComputedY, tol)
        << "footer no longer bottom-anchored after sibling-only relayout";
    EXPECT_NEAR(navH0, navRows[0]->GetLayout().ComputedHeight, tol)
        << "nav row lost its height after sibling-only relayout";
    EXPECT_NEAR(navGap0, navRows[1]->GetLayout().ComputedY - navRows[0]->GetLayout().ComputedY, tol)
        << "nav rows overlap (lost gap) after sibling-only relayout";
}

// Horizontal analog of the sidebar collapse — the header bug in bad.png. The header Row holds
// a flex-grow Spacer that pushes a fixed search box + button to the RIGHT. Crucially the header
// Row's WIDTH comes from CROSS-axis stretch (it is the cross child of the Header column), not
// main-axis grow — a different path than the sidebar's vertical spacer. A sibling-only relayout
// (the content area) must leave the clean header pinned right.
TEST(FlexTests, partial_relayout_keeps_clean_header_spacer_pushed_right) {
    const CSSValue autoSize{};
    auto px = [](float v) { return CSSValue(v, CSSUnit::Px); };
    auto setBorder = [&](const NodePtr &n, float w) {
        auto &b = n->GetStyle().Modify<BorderProperties>();
        b.WidthTop = b.WidthBottom = b.WidthLeft = b.WidthRight = px(w);
    };
    auto setGap = [&](const NodePtr &n, float g) {
        auto &f = n->GetStyle().Modify<CSSFlex>();
        f.Gaps.Row = px(g);
        f.Gaps.Column = px(g);
    };
    auto setAlign = [](const NodePtr &n, AlignItems a) { n->GetStyle().Modify<CSSFlex>().Align = a; };
    auto setExpandGrow = [&](const NodePtr &n, float g) {
        auto &f = n->GetStyle().Modify<CSSFlex>();
        f.FlexGrow = g;
        f.FlexBasis = px(0.0f);
    };
    auto setPaddingLR = [&](const NodePtr &n, float lr) {
        auto &p = n->GetStyle().Modify<PaddingEdge>();
        p.Left = p.Right = px(lr);
    };

    const float W = 1024.0f, H = 768.0f;

    // root(col, pinned) > appRow(row, grow-h, stretch) > [ sidebar(220), content(grow-w) ]
    auto root = flexCol();
    setSize(root, px(W), px(H));

    auto appRow = flexRow();
    setSize(appRow, autoSize, autoSize);
    setExpandGrow(appRow, 1.0f);
    setAlign(appRow, AlignItems::Stretch);
    root->AddChild(appRow);

    auto sidebar = flexCol();                        // fixed 220 -> content area is 804 wide
    setSize(sidebar, px(220.0f), autoSize);
    appRow->AddChild(sidebar);
    auto sbChild = flexCol();
    setSize(sbChild, autoSize, px(100.0f));
    sidebar->AddChild(sbChild);

    auto content = flexCol();                        // grows to fill the remaining width
    setSize(content, autoSize, autoSize);
    setExpandGrow(content, 1.0f);
    setAlign(content, AlignItems::Stretch);
    appRow->AddChild(content);

    // header: Box -> column, fixed 68 height, AUTO (cross-stretched) width, 1px border.
    auto header = flexCol();
    setSize(header, autoSize, px(68.0f));
    setBorder(header, 1.0f);
    setAlign(header, AlignItems::Stretch);
    content->AddChild(header);

    // headerRow: fills the 68px height (grow), width comes from Header's cross-stretch.
    auto headerRow = flexRow();
    setSize(headerRow, autoSize, autoSize);
    setExpandGrow(headerRow, 1.0f);
    setGap(headerRow, 16.0f);
    setPaddingLR(headerRow, 28.0f);
    setAlign(headerRow, AlignItems::FlexCenter);
    header->AddChild(headerRow);

    auto title = flexCol();                          // AUTO width, shrink-to-fit to its child
    setSize(title, autoSize, autoSize);
    headerRow->AddChild(title);
    auto titleChild = flexCol();
    setSize(titleChild, px(120.0f), px(22.0f));
    title->AddChild(titleChild);

    auto spacer = flexRow();                         // flex-grow spacer pushes the rest right
    setSize(spacer, autoSize, autoSize);
    setExpandGrow(spacer, 1.0f);
    headerRow->AddChild(spacer);

    auto search = flexRow();                         // fixed 220 search box
    setSize(search, px(220.0f), px(36.0f));
    headerRow->AddChild(search);

    auto button = flexCol();                         // AUTO width button
    setSize(button, autoSize, autoSize);
    headerRow->AddChild(button);
    auto buttonChild = flexCol();
    setSize(buttonChild, px(70.0f), px(34.0f));
    button->AddChild(buttonChild);

    // contentArea: the sibling we dirty (stands in for the animating chart).
    auto contentArea = flexCol();
    setSize(contentArea, autoSize, autoSize);
    setExpandGrow(contentArea, 1.0f);
    setAlign(contentArea, AlignItems::Stretch);
    content->AddChild(contentArea);
    auto areaChild = flexCol();
    setSize(areaChild, autoSize, px(120.0f));
    contentArea->AddChild(areaChild);

    // ── Initial solve ─────────────────────────────────────────────────────────
    root->Calculate(W, H);

    // content area spans x∈[220,1024]; header content padding 28 + border 1, so the search box's
    // right edge should be near ~995 and the spacer should have eaten most of the row.
    EXPECT_GT(spacer->GetLayout().ComputedWidth, 150.0f) << "spacer not grown initially";
    EXPECT_GT(search->GetLayout().ComputedX + search->GetLayout().ComputedWidth, 700.0f)
        << "search box not pushed right initially";

    const float tol = 1.0f;
    const float spacerW0 = spacer->GetLayout().ComputedWidth;
    const float searchX0  = search->GetLayout().ComputedX;
    const float buttonX0  = button->GetLayout().ComputedX;

    auto dump = [&](const char* phase) {
        std::cout << "[" << phase << "] content.w=" << content->GetLayout().ComputedWidth
                  << " header.w=" << header->GetLayout().ComputedWidth
                  << " headerRow.w=" << headerRow->GetLayout().ComputedWidth
                  << " headerRow.h=" << headerRow->GetLayout().ComputedHeight
                  << " spacer.w=" << spacer->GetLayout().ComputedWidth
                  << " title.w=" << title->GetLayout().ComputedWidth
                  << " search.w=" << search->GetLayout().ComputedWidth
                  << " search.x=" << search->GetLayout().ComputedX << "\n";
    };
    dump("initial");

    // ── Partial relayout: dirty ONLY the content-area subtree ──
    auto newAreaChild = flexCol();
    setSize(newAreaChild, autoSize, px(140.0f));
    contentArea->ClearChildren();
    contentArea->AddChild(newAreaChild);
    contentArea->MarkDirtyToRoot();

    root->Calculate(W, H);
    dump("relayout");

    // The clean header must be unchanged: spacer still wide, search/button still pinned right.
    EXPECT_NEAR(spacerW0, spacer->GetLayout().ComputedWidth, tol)
        << "header spacer collapsed after sibling-only relayout";
    EXPECT_NEAR(searchX0, search->GetLayout().ComputedX, tol)
        << "search box shifted left after sibling-only relayout";
    EXPECT_NEAR(buttonX0, button->GetLayout().ComputedX, tol)
        << "button shifted left after sibling-only relayout";
}

// Third facet of the relayout bug: a CENTERED element drifts (the sidebar logo's accent box
// "moves to the right"). The logo Row is centered horizontally inside the logo Box
// (align-items: center); the accent box is the Row's first child. A sibling-only relayout must
// not move it — its X is driven by the row's width and the box's content width via the
// centering math, so a stale size there shifts the whole row.
TEST(FlexTests, partial_relayout_keeps_clean_centered_logo_in_place) {
    const CSSValue autoSize{};
    auto px = [](float v) { return CSSValue(v, CSSUnit::Px); };
    auto setGap = [&](const NodePtr &n, float g) {
        auto &f = n->GetStyle().Modify<CSSFlex>();
        f.Gaps.Row = px(g);
        f.Gaps.Column = px(g);
    };
    auto setAlign = [](const NodePtr &n, AlignItems a) { n->GetStyle().Modify<CSSFlex>().Align = a; };
    auto setJustify = [](const NodePtr &n, JustifyContent j) { n->GetStyle().Modify<CSSFlex>().Justify = j; };
    auto setExpandGrow = [&](const NodePtr &n, float g) {
        auto &f = n->GetStyle().Modify<CSSFlex>();
        f.FlexGrow = g;
        f.FlexBasis = px(0.0f);
    };
    auto setPadding4 = [&](const NodePtr &n, float t, float r, float b, float l) {
        auto &p = n->GetStyle().Modify<PaddingEdge>();
        p.Top = px(t); p.Right = px(r); p.Bottom = px(b); p.Left = px(l);
    };
    auto setPaddingAll = [&](const NodePtr &n, float a) {
        auto &p = n->GetStyle().Modify<PaddingEdge>();
        p.Top = p.Right = p.Bottom = p.Left = px(a);
    };
    auto setBorder = [&](const NodePtr &n, float w) {
        auto &b = n->GetStyle().Modify<BorderProperties>();
        b.WidthTop = b.WidthBottom = b.WidthLeft = b.WidthRight = px(w);
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
    root->AddChild(appRow);

    // sidebar Box: fixed 220 wide, 1px border, stretches its child column.
    auto sidebar = flexCol();
    setSize(sidebar, px(220.0f), autoSize);
    setBorder(sidebar, 1.0f);
    setAlign(sidebar, AlignItems::Stretch);
    appRow->AddChild(sidebar);

    // inner column: Expand height (grow), cross=stretch, padding 12, gap 6 — exactly the
    // wrapper that holds the demo logo, so the logo sits two stretch levels deep.
    auto innerCol = flexCol();
    setSize(innerCol, autoSize, autoSize);
    setExpandGrow(innerCol, 1.0f);
    setPaddingAll(innerCol, 12.0f);
    setGap(innerCol, 6.0f);
    setAlign(innerCol, AlignItems::Stretch);
    sidebar->AddChild(innerCol);

    // logo Box: column, AUTO size (cross-stretched), padding {14,12,14,12}, centered both ways.
    auto logoBox = flexCol();
    setSize(logoBox, autoSize, autoSize);
    setPadding4(logoBox, 14.0f, 12.0f, 14.0f, 12.0f);
    setCenterChild(logoBox);
    innerCol->AddChild(logoBox);

    auto logoRow = flexRow();                        // shrink-to-fit row, centered in logoBox
    setSize(logoRow, autoSize, autoSize);
    setGap(logoRow, 12.0f);
    setAlign(logoRow, AlignItems::FlexCenter);
    logoBox->AddChild(logoRow);

    auto accent = flexCol();                         // the box that "moves right"; 28x28, centers a dot
    setSize(accent, px(28.0f), px(28.0f));
    setCenterChild(accent);
    logoRow->AddChild(accent);
    auto dot = flexCol();
    setSize(dot, px(14.0f), px(14.0f));
    accent->AddChild(dot);
    auto label = flexCol();                          // stands in for the "Lumora" text
    setSize(label, px(90.0f), px(20.0f));
    logoRow->AddChild(label);

    auto navFiller = flexCol();                      // fills the rest of the inner column height
    setSize(navFiller, autoSize, autoSize);
    setExpandGrow(navFiller, 1.0f);
    innerCol->AddChild(navFiller);

    auto content = flexCol();
    setSize(content, autoSize, autoSize);
    setExpandGrow(content, 1.0f);
    setAlign(content, AlignItems::Stretch);
    appRow->AddChild(content);

    auto contentArea = flexCol();
    setSize(contentArea, autoSize, autoSize);
    setExpandGrow(contentArea, 1.0f);
    setAlign(contentArea, AlignItems::Stretch);
    content->AddChild(contentArea);
    auto areaChild = flexCol();
    setSize(areaChild, autoSize, px(120.0f));
    contentArea->AddChild(areaChild);

    auto dump = [&](const char* phase) {
        std::cout << "[" << phase << "] logoBox.w=" << logoBox->GetLayout().ComputedWidth
                  << " logoRow.w=" << logoRow->GetLayout().ComputedWidth
                  << " logoRow.x=" << logoRow->GetLayout().ComputedX
                  << " accent.x=" << accent->GetLayout().ComputedX
                  << " accent.w=" << accent->GetLayout().ComputedWidth << "\n";
    };

    // ── Initial solve ─────────────────────────────────────────────────────────
    root->Calculate(W, H);
    dump("initial");

    const float tol = 1.0f;
    const float accentX0  = accent->GetLayout().ComputedX;
    const float logoRowX0 = logoRow->GetLayout().ComputedX;
    const float logoRowW0 = logoRow->GetLayout().ComputedWidth;

    EXPECT_GT(accentX0, 12.0f) << "logo not centered initially (should be inset past the padding)";

    // ── Partial relayout: dirty ONLY the content-area subtree ──
    auto newAreaChild = flexCol();
    setSize(newAreaChild, autoSize, px(140.0f));
    contentArea->ClearChildren();
    contentArea->AddChild(newAreaChild);
    contentArea->MarkDirtyToRoot();

    root->Calculate(W, H);
    dump("relayout");

    EXPECT_NEAR(accentX0, accent->GetLayout().ComputedX, tol)
        << "accent box drifted horizontally after sibling-only relayout";
    EXPECT_NEAR(logoRowX0, logoRow->GetLayout().ComputedX, tol)
        << "logo row drifted after sibling-only relayout";
    EXPECT_NEAR(logoRowW0, logoRow->GetLayout().ComputedWidth, tol)
        << "logo row width changed after sibling-only relayout";
}

TEST(FlexTests, flex_wrap_wrap) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Wrap = FlexWrap::Wrap;
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto child0 = std::make_shared<Node>();
    child0->GetStyle().Modify<Dimensions>().Width = 60.0f;
    child0->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(child0);

    auto child1 = std::make_shared<Node>();
    child1->GetStyle().Modify<Dimensions>().Width = 60.0f;
    child1->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(child1);

    root->Calculate(100, 100);

    ASSERT_FLOAT_EQ(0.0f, child0->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(0.0f, child0->GetLayout().ComputedY);

    ASSERT_FLOAT_EQ(0.0f, child1->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(40.0f, child1->GetLayout().ComputedY);
}

// Guard for the CSS `order` sort branch in CollectAndOrderItems (previously untested):
// a child with a larger `order` must be placed after one with a smaller `order`,
// regardless of DOM sequence.
TEST(FlexTests, order_overrides_dom_sequence) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 100.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto a = std::make_shared<Node>(); // added first, but order 2
    a->GetStyle().Modify<Dimensions>().Width = 30.0f;
    a->GetStyle().Modify<Dimensions>().Height = 30.0f;
    a->GetStyle().Modify<CSSFlex>().Order = 2;
    root->AddChild(a);

    auto b = std::make_shared<Node>(); // added second, but order 1
    b->GetStyle().Modify<Dimensions>().Width = 30.0f;
    b->GetStyle().Modify<Dimensions>().Height = 30.0f;
    b->GetStyle().Modify<CSSFlex>().Order = 1;
    root->AddChild(b);

    root->Calculate(100.0f, 100.0f);

    // ascending order => b (1) before a (2)
    ASSERT_FLOAT_EQ(0.0f, b->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(30.0f, a->GetLayout().ComputedX);
}

// Guard for percent cross-axis margin resolution in AlignLinesOnCrossAxis (previously
// untested): margin-top is the cross-start edge of a row, resolved against the line's
// cross size (the definite container height here = 100), so 10% => 10px offset.
TEST(FlexTests, percent_cross_margin_offsets_item) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex); // default direction Row
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 100.0f;

    auto child = std::make_shared<Node>();
    child->GetStyle().Modify<Dimensions>().Width = 50.0f;
    child->GetStyle().Modify<Dimensions>().Height = 30.0f; // explicit => no stretch resize
    child->GetStyle().Modify<MarginEdge>().Top = CSSValue(10.0f, CSSUnit::Percent);
    root->AddChild(child);

    root->Calculate(200.0f, 100.0f);

    ASSERT_FLOAT_EQ(0.0f, child->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(10.0f, child->GetLayout().ComputedY); // 10% of line cross size (100)
    ASSERT_FLOAT_EQ(50.0f, child->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(30.0f, child->GetLayout().ComputedHeight);
}
