#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

using namespace masharif;


TEST(OutOfFlowRepositionTests, absolute_child_follows_moved_containing_block) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    // A preceding sibling whose height changes between solves: growing it pushes the
    // containing block down without changing the block's own size or marking it dirty.
    auto spacer = std::make_shared<Node>();
    spacer->GetStyle().Modify<Dimensions>().Width = 200.0f;
    spacer->GetStyle().Modify<Dimensions>().Height = 30.0f;
    root->AddChild(spacer);

    // Relative => a containing block for its absolute descendants, still laid out in flow.
    auto cb = std::make_shared<Node>();
    cb->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    cb->GetStyle().Modify<Dimensions>().Width = 100.0f;
    cb->GetStyle().Modify<Dimensions>().Height = 50.0f;
    root->AddChild(cb);

    auto abs = std::make_shared<Node>();
    abs->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    abs->GetStyle().Modify<Dimensions>().Left = 0.0f;
    abs->GetStyle().Modify<Dimensions>().Top = 0.0f;
    abs->GetStyle().Modify<Dimensions>().Width = 20.0f;
    abs->GetStyle().Modify<Dimensions>().Height = 20.0f;
    cb->AddChild(abs);

    // First solve: the spacer is 30 tall, so the block sits at y=30 and the absolute child
    // pins to its top-left.
    root->Calculate(200.0f, 200.0f);
    ASSERT_FLOAT_EQ(30.0f, cb->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(30.0f, abs->GetLayout().ComputedY);
    ASSERT_FLOAT_EQ(0.0f, abs->GetLayout().ComputedX);

    // Move-only frame: only the spacer is dirtied. The block keeps its size (so its layout
    // strategy does not re-run), but it slides to y=80 — the absolute child must follow.
    spacer->GetStyle().Modify<Dimensions>().Height = 80.0f;
    root->Calculate(200.0f, 200.0f);

    ASSERT_FLOAT_EQ(80.0f, cb->GetLayout().ComputedY);   // the block moved
    ASSERT_FLOAT_EQ(80.0f, abs->GetLayout().ComputedY);  // child followed (froze at 30 before the fix)
    ASSERT_FLOAT_EQ(0.0f, abs->GetLayout().ComputedX);
}

// display:none generates no box: an out-of-flow child must not be collected, measured, or
// positioned at its insets — it stays at the default zero geometry (before the fix it was
// pushed to m_OutOfFlowChildren and positioned at Left/Top = 50,50).
TEST(OutOfFlowRepositionTests, display_none_absolute_child_is_not_positioned) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto abs = std::make_shared<Node>();
    abs->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    abs->GetStyle().Modify<Dimensions>().Display = OuterDisplay::None;
    abs->GetStyle().Modify<Dimensions>().Left = 50.0f;
    abs->GetStyle().Modify<Dimensions>().Top = 50.0f;
    abs->GetStyle().Modify<Dimensions>().Width = 20.0f;
    abs->GetStyle().Modify<Dimensions>().Height = 20.0f;
    root->AddChild(abs);

    root->Calculate(200.0f, 200.0f);

    EXPECT_FLOAT_EQ(0.0f, abs->GetLayout().ComputedX);
    EXPECT_FLOAT_EQ(0.0f, abs->GetLayout().ComputedY);
    EXPECT_FLOAT_EQ(0.0f, abs->GetLayout().ComputedWidth);
    EXPECT_FLOAT_EQ(0.0f, abs->GetLayout().ComputedHeight);
}

// A containing block turned display:none must not re-position its absolute child. The child is
// right-anchored, so the bug (positioning it against the block's now-zero size) would move it
// from x=70 to x=-30; the hidden subtree must be left untouched, then re-anchored once shown.
TEST(OutOfFlowRepositionTests, hidden_containing_block_does_not_reposition_abs_child) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto cb = std::make_shared<Node>();
    cb->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    cb->GetStyle().Modify<Dimensions>().Width = 100.0f;
    cb->GetStyle().Modify<Dimensions>().Height = 50.0f;
    root->AddChild(cb);

    auto abs = std::make_shared<Node>();
    abs->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    // Right-anchored: Left must be AUTO (insets default to 0px, which would left-anchor), so the
    // X position depends on the containing block's width — the value the bug would corrupt.
    abs->GetStyle().Modify<Dimensions>().Left = CSSValue(0.0f, CSSUnit::Auto);
    abs->GetStyle().Modify<Dimensions>().Right = 10.0f;
    abs->GetStyle().Modify<Dimensions>().Top = 0.0f;
    abs->GetStyle().Modify<Dimensions>().Width = 20.0f;
    abs->GetStyle().Modify<Dimensions>().Height = 20.0f;
    cb->AddChild(abs);

    root->Calculate(200.0f, 200.0f);
    ASSERT_FLOAT_EQ(70.0f, abs->GetLayout().ComputedX); // cb(100) - right(10) - width(20)

    cb->GetStyle().Modify<Dimensions>().Display = OuterDisplay::None;
    root->Calculate(200.0f, 200.0f);
    EXPECT_FLOAT_EQ(70.0f, abs->GetLayout().ComputedX); // not repositioned while hidden

    cb->GetStyle().Modify<Dimensions>().Display = OuterDisplay::Block;
    root->Calculate(200.0f, 200.0f);
    EXPECT_FLOAT_EQ(70.0f, abs->GetLayout().ComputedX); // re-anchored once visible again
}

// Nested case: a display:none ANCESTOR with a still-visible child that owns an absolute
// grandchild. The visible child's strategy cannot run inside the hidden subtree, so its
// out-of-flow list keeps a dangling raw pointer once the grandchild is freed — the walk must
// not descend into the hidden subtree. Reaching the end without a crash is the assertion.
TEST(OutOfFlowRepositionTests, hidden_ancestor_does_not_walk_stale_nested_out_of_flow_list) {
    auto root = std::make_shared<Node>();
    root->GetStyle().Modify<Dimensions>().Width = 200.0f;
    root->GetStyle().Modify<Dimensions>().Height = 200.0f;

    auto outer = std::make_shared<Node>(); // the node we hide
    outer->GetStyle().Modify<Dimensions>().Width = 200.0f;
    outer->GetStyle().Modify<Dimensions>().Height = 100.0f;
    root->AddChild(outer);

    auto cb = std::make_shared<Node>(); // visible mid-node, containing block for the grandchild
    cb->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
    cb->GetStyle().Modify<Dimensions>().Width = 100.0f;
    cb->GetStyle().Modify<Dimensions>().Height = 50.0f;
    outer->AddChild(cb);

    auto abs = std::make_shared<Node>();
    abs->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
    abs->GetStyle().Modify<Dimensions>().Left = 0.0f;
    abs->GetStyle().Modify<Dimensions>().Top = 0.0f;
    abs->GetStyle().Modify<Dimensions>().Width = 20.0f;
    abs->GetStyle().Modify<Dimensions>().Height = 20.0f;
    cb->AddChild(abs);

    root->Calculate(200.0f, 200.0f); // populates cb.m_OutOfFlowChildren with abs's raw pointer

    // Hide the ancestor, then free the nested absolute child. cb's strategy will not run while
    // its ancestor is hidden, so cb's out-of-flow list keeps the now-dangling raw pointer.
    outer->GetStyle().Modify<Dimensions>().Display = OuterDisplay::None;
    cb->ClearChildren();
    abs.reset(); // drop the last owning reference -> the Node is destroyed

    root->Calculate(200.0f, 200.0f); // must not descend into the hidden subtree / deref the pointer

    EXPECT_FLOAT_EQ(0.0f, cb->GetLayout().ComputedX); // visible tree still sane; no crash
}
