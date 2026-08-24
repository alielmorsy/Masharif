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
