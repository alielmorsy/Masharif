#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;

// Normal-flow (display:block) was never exercised by the Lumora backend until CSS `display:block`
// began routing to it. These tests pin the block behaviour Lumora relies on: children stack
// vertically, take the container's content width (not the raw viewport), display:none generates no
// box, and a bottom margin spaces the next sibling.

TEST(NormalFlowTests, block_children_stack_and_fill_content_width) {
    auto root = std::make_shared<Node>(OuterDisplay::Block);
    root->GetStyle().Modify<Dimensions>().Width = 200.0f; // narrower than the viewport below

    auto make = [](float h) {
        auto n = std::make_shared<Node>(OuterDisplay::Block);
        n->GetStyle().Modify<Dimensions>().Height = h; // width stays auto → fills the content box
        return n;
    };
    auto c0 = make(40.0f);
    auto c1 = make(40.0f);
    auto c2 = make(40.0f);
    root->AddChild(c0);
    root->AddChild(c1);
    root->AddChild(c2);

    // Viewport far wider/taller than the container: children must size to the 200px container, not 1000.
    root->Calculate(1000, 1000);

    ASSERT_FLOAT_EQ(200.0f, root->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(120.0f, root->GetLayout().ComputedHeight); // auto height sums the three children

    ASSERT_FLOAT_EQ(200.0f, c0->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(200.0f, c1->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(200.0f, c2->GetLayout().ComputedWidth);

    ASSERT_FLOAT_EQ(0.0f, c0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(40.0f, c1->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(80.0f, c2->GetLayout().ComputedY);
}

TEST(NormalFlowTests, block_display_none_generates_no_box) {
    auto root = std::make_shared<Node>(OuterDisplay::Block);
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;

    auto c0 = std::make_shared<Node>(OuterDisplay::Block);
    c0->GetStyle().Modify<Dimensions>().Height = 40.0f;
    auto hidden = std::make_shared<Node>(OuterDisplay::None);
    hidden->GetStyle().Modify<Dimensions>().Height = 40.0f;
    auto c2 = std::make_shared<Node>(OuterDisplay::Block);
    c2->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(c0);
    root->AddChild(hidden);
    root->AddChild(c2);

    root->Calculate(1000, 1000);

    ASSERT_FLOAT_EQ(0.0f, c0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(40.0f, c2->GetLayout().ComputedY); // hidden node did not advance the flow
    ASSERT_FLOAT_EQ(80.0f, root->GetLayout().ComputedHeight);
}

TEST(NormalFlowTests, block_bottom_margin_spaces_next_sibling) {
    auto root = std::make_shared<Node>(OuterDisplay::Block);
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;

    auto c0 = std::make_shared<Node>(OuterDisplay::Block);
    c0->GetStyle().Modify<Dimensions>().Height = 40.0f;
    c0->GetStyle().Modify<MarginEdge>().Bottom = 10.0f;
    auto c1 = std::make_shared<Node>(OuterDisplay::Block);
    c1->GetStyle().Modify<Dimensions>().Height = 30.0f;
    root->AddChild(c0);
    root->AddChild(c1);

    root->Calculate(1000, 1000);

    ASSERT_FLOAT_EQ(0.0f, c0->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(50.0f, c1->GetLayout().ComputedY); // 40 height + 10 bottom margin
}

// --- gh#3: a block-level AUTO-width flex container in normal flow fills, it does not shrink-wrap ---
//
// CSS reserves shrink-to-fit for floats, inline-level boxes and absolutely positioned boxes. A
// block-level box with `width: auto` fills its containing block. FlexLayoutStrategy's shrink-wrap
// branch is gated on MainSizeIsDefinite(), which NormalFlowStrategy never set — so every
// `display:flex` child reached through normal flow collapsed to its content width, taking its whole
// subtree with it (grow items resolved against a 0-wide container).

TEST(NormalFlowTests, block_level_flex_child_fills_container_width) {
    auto root = std::make_shared<Node>(OuterDisplay::Block);
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;

    auto row = std::make_shared<Node>(OuterDisplay::Flex); // width stays auto
    row->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(row);

    std::shared_ptr<Node> items[2];
    for (auto &item: items) {
        item = std::make_shared<Node>();
        item->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
        item->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(0.0f);
        row->AddChild(item);
    }

    root->Calculate(1000.0f, 1000.0f);

    ASSERT_FLOAT_EQ(300.0f, row->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(150.0f, items[0]->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(150.0f, items[1]->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(0.0f, items[0]->GetLayout().ComputedX);
    ASSERT_FLOAT_EQ(150.0f, items[1]->GetLayout().ComputedX);
}

TEST(NormalFlowTests, block_level_flex_child_fills_content_box_inside_padding_and_border) {
    auto root = std::make_shared<Node>(OuterDisplay::Block);
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<PaddingEdge>().Left = CSSValue(10.0f);
    root->GetStyle().Modify<PaddingEdge>().Right = CSSValue(10.0f);

    auto row = std::make_shared<Node>(OuterDisplay::Flex);
    row->GetStyle().Modify<Dimensions>().Height = 40.0f;
    row->GetStyle().Modify<PaddingEdge>().Left = CSSValue(15.0f);
    row->GetStyle().Modify<MarginEdge>().Left = CSSValue(20.0f);
    root->AddChild(row);

    auto item = std::make_shared<Node>();
    item->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    item->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(0.0f);
    row->AddChild(item);

    root->Calculate(1000.0f, 1000.0f);

    // Root content box 280, less the row's own 20px margin-left -> 260 border box.
    ASSERT_FLOAT_EQ(260.0f, row->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(30.0f, row->GetLayout().ComputedX); // 10 padding + 20 margin
    // The single grow item fills the row's content box: 260 less its 15px padding-left.
    ASSERT_FLOAT_EQ(245.0f, item->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(45.0f, item->GetLayout().ComputedX);
}

TEST(NormalFlowTests, block_level_column_flex_child_fills_width_but_not_height) {
    auto root = std::make_shared<Node>(OuterDisplay::Block);
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;
    root->GetStyle().Modify<Dimensions>().Height = 500.0f;

    auto col = std::make_shared<Node>(OuterDisplay::Flex); // width AND height stay auto
    col->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->AddChild(col);

    std::shared_ptr<Node> items[2];
    for (auto &item: items) {
        item = std::make_shared<Node>();
        item->GetStyle().Modify<Dimensions>().Height = 30.0f;
        col->AddChild(item);
    }

    root->Calculate(1000.0f, 1000.0f);

    ASSERT_FLOAT_EQ(300.0f, col->GetLayout().ComputedWidth);
    // The main axis is vertical here: an AUTO block-axis size is still content-sized, so the
    // fill must not leak onto it.
    ASSERT_FLOAT_EQ(60.0f, col->GetLayout().ComputedHeight);
}

TEST(NormalFlowTests, inline_flex_child_still_shrink_wraps) {
    auto root = std::make_shared<Node>(OuterDisplay::Block);
    root->GetStyle().Modify<Dimensions>().Width = 300.0f;

    auto row = std::make_shared<Node>(OuterDisplay::InlineFlex); // inline-level -> shrink-to-fit
    row->GetStyle().Modify<Dimensions>().Height = 40.0f;
    root->AddChild(row);

    std::shared_ptr<Node> items[2];
    for (auto &item: items) {
        item = std::make_shared<Node>();
        item->GetStyle().Modify<Dimensions>().Width = 50.0f;
        row->AddChild(item);
    }

    root->Calculate(1000.0f, 1000.0f);

    ASSERT_FLOAT_EQ(100.0f, row->GetLayout().ComputedWidth);
}
