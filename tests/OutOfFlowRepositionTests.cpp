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
