// Standalone comparison harness for Masharif layout engine vs browser ground truth.
// Builds a fixed set of trees, calls Calculate(), and dumps computed absolute
// {x,y,w,h} per named node as one JSON object keyed by fixture name.
//
// Compile (no CMake needed, masharifcore is header+source only):
//   clang++ -std=c++20 -O2 -I <repo_root> harness.cpp <repo_root>/masharifcore/layout/*.cpp
//           <repo_root>/masharifcore/structure/*.cpp -o harness.exe

#include <masharifcore/Masharif.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

using namespace masharif;

namespace {
    struct Rec { std::string id; SharedNode node; };
    std::vector<Rec> g_records;

    SharedNode N(const std::string &id, OuterDisplay display = OuterDisplay::Block) {
        auto n = std::make_shared<Node>(display);
        g_records.push_back({id, n});
        return n;
    }

    void PrintFixture(const std::string &name) {
        std::cout << "\"" << name << "\":{";
        bool first = true;
        for (auto &r : g_records) {
            if (!first) std::cout << ",";
            first = false;
            auto &l = r.node->GetLayout();
            std::cout << "\"" << r.id << "\":{"
                      << "\"x\":" << l.ComputedX << ","
                      << "\"y\":" << l.ComputedY << ","
                      << "\"w\":" << l.ComputedWidth << ","
                      << "\"h\":" << l.ComputedHeight << "}";
        }
        std::cout << "}";
        g_records.clear();
    }

    // ---------------------------------------------------------------- fixtures

    void Fixture_flex_row_justify_between(float &W, float &H) {
        W = 400; H = 120;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        root->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::SpaceBetween;
        root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;
        for (int i = 0; i < 3; ++i) {
            auto c = N("c" + std::to_string(i));
            c->GetStyle().Modify<Dimensions>().Width = 60.0f;
            c->GetStyle().Modify<Dimensions>().Height = 40.0f + i * 10.0f;
            root->AddChild(c);
        }
        root->Calculate(W, H);
    }

    void Fixture_flex_wrap_align_content(float &W, float &H) {
        W = 130; H = 200;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        root->GetStyle().Modify<CSSFlex>().Wrap = FlexWrap::Wrap;
        root->GetStyle().Modify<CSSFlex>().ContentAlign = AlignContent::SpaceAround;
        root->GetStyle().Modify<CSSFlex>().Gaps.Row = CSSValue(10.0f);
        root->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(5.0f);
        for (int i = 0; i < 4; ++i) {
            auto c = N("c" + std::to_string(i));
            c->GetStyle().Modify<Dimensions>().Width = 50.0f;
            c->GetStyle().Modify<Dimensions>().Height = 30.0f;
            root->AddChild(c);
        }
        root->Calculate(W, H);
    }

    void Fixture_flex_grow_shrink_minmax_clamp(float &W, float &H) {
        W = 300; H = 80;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;

        auto a = N("a");
        a->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
        a->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(0.0f);
        a->GetStyle().Modify<Dimensions>().MaxWidth = 80.0f;
        root->AddChild(a);

        auto b = N("b");
        b->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
        b->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(0.0f);
        b->GetStyle().Modify<Dimensions>().MinWidth = 120.0f;
        root->AddChild(b);

        auto c = N("c");
        c->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
        c->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(0.0f);
        root->AddChild(c);

        root->Calculate(W, H);
    }

    void Fixture_flex_column_percent_width_height(float &W, float &H) {
        W = 300; H = 200;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;

        auto c0 = N("c0");
        c0->GetStyle().Modify<Dimensions>().Width = CSSValue(50.0f, CSSUnit::Percent);
        c0->GetStyle().Modify<Dimensions>().Height = CSSValue(50.0f, CSSUnit::Percent);
        root->AddChild(c0);

        auto c1 = N("c1");
        c1->GetStyle().Modify<Dimensions>().Width = CSSValue(75.0f, CSSUnit::Percent);
        c1->GetStyle().Modify<Dimensions>().Height = 40.0f;
        root->AddChild(c1);

        root->Calculate(W, H);
    }

    void Fixture_align_items_variants(float &W, float &H) {
        W = 320; H = 100;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexStart;

        auto c0 = N("c0"); // inherits flex-start
        c0->GetStyle().Modify<Dimensions>().Width = 40.0f;
        c0->GetStyle().Modify<Dimensions>().Height = 30.0f;
        root->AddChild(c0);

        auto c1 = N("c1"); // align-self: center
        c1->GetStyle().Modify<Dimensions>().Width = 40.0f;
        c1->GetStyle().Modify<Dimensions>().Height = 30.0f;
        c1->GetStyle().Modify<CSSFlex>().AlignSelf = AlignItems::FlexCenter;
        root->AddChild(c1);

        auto c2 = N("c2"); // align-self: flex-end
        c2->GetStyle().Modify<Dimensions>().Width = 40.0f;
        c2->GetStyle().Modify<Dimensions>().Height = 30.0f;
        c2->GetStyle().Modify<CSSFlex>().AlignSelf = AlignItems::FlexEnd;
        root->AddChild(c2);

        auto c3 = N("c3"); // align-self: stretch, auto height
        c3->GetStyle().Modify<Dimensions>().Width = 40.0f;
        c3->GetStyle().Modify<CSSFlex>().AlignSelf = AlignItems::Stretch;
        root->AddChild(c3);

        root->Calculate(W, H);
    }

    // Targeted bug check: align-items:baseline. Two items with different top padding
    // so their text baselines would differ from their box tops in a real browser.
    void Fixture_align_items_baseline(float &W, float &H) {
        W = 200; H = 100;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        root->GetStyle().Modify<CSSFlex>().Align = AlignItems::Baseline;

        auto c0 = N("c0");
        c0->GetStyle().Modify<Dimensions>().Width = 40.0f;
        c0->GetStyle().Modify<Dimensions>().Height = 60.0f;
        root->AddChild(c0);

        auto c1 = N("c1");
        c1->GetStyle().Modify<Dimensions>().Width = 40.0f;
        c1->GetStyle().Modify<Dimensions>().Height = 30.0f;
        root->AddChild(c1);

        root->Calculate(W, H);
    }

    void Fixture_order_reorder(float &W, float &H) {
        W = 240; H = 60;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;

        auto cA = N("cA"); // DOM order 1st, CSS order 3
        cA->GetStyle().Modify<Dimensions>().Width = 60.0f;
        cA->GetStyle().Modify<Dimensions>().Height = 40.0f;
        cA->GetStyle().Modify<CSSFlex>().Order = 3;
        root->AddChild(cA);

        auto cB = N("cB"); // DOM order 2nd, CSS order 1
        cB->GetStyle().Modify<Dimensions>().Width = 60.0f;
        cB->GetStyle().Modify<Dimensions>().Height = 40.0f;
        cB->GetStyle().Modify<CSSFlex>().Order = 1;
        root->AddChild(cB);

        auto cC = N("cC"); // DOM order 3rd, CSS order 2
        cC->GetStyle().Modify<Dimensions>().Width = 60.0f;
        cC->GetStyle().Modify<Dimensions>().Height = 40.0f;
        cC->GetStyle().Modify<CSSFlex>().Order = 2;
        root->AddChild(cC);

        root->Calculate(W, H);
    }

    void Fixture_gap_row_and_column(float &W, float &H) {
        W = 250; H = 150;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        root->GetStyle().Modify<CSSFlex>().Wrap = FlexWrap::Wrap;
        root->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(20.0f); // between items on a line
        root->GetStyle().Modify<CSSFlex>().Gaps.Row = CSSValue(15.0f);    // between lines
        for (int i = 0; i < 4; ++i) {
            auto c = N("c" + std::to_string(i));
            c->GetStyle().Modify<Dimensions>().Width = 90.0f;
            c->GetStyle().Modify<Dimensions>().Height = 40.0f;
            root->AddChild(c);
        }
        root->Calculate(W, H);
    }

    // Targeted bug check: percentage margin-top on a COLUMN flex item. CSS resolves
    // ALL percentage margins (including top/bottom) against the containing block's
    // WIDTH, never its height. Container width(400) != height(150) makes the two
    // possible reference axes give different, unambiguous numbers.
    void Fixture_margin_percent_reference_axis(float &W, float &H) {
        W = 400; H = 150;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;

        auto c0 = N("c0");
        c0->GetStyle().Modify<Dimensions>().Width = 50.0f;
        c0->GetStyle().Modify<Dimensions>().Height = 30.0f;
        c0->GetStyle().Modify<MarginEdge>().Top = CSSValue(10.0f, CSSUnit::Percent);
        root->AddChild(c0);

        auto c1 = N("c1");
        c1->GetStyle().Modify<Dimensions>().Width = 50.0f;
        c1->GetStyle().Modify<Dimensions>().Height = 30.0f;
        root->AddChild(c1);

        root->Calculate(W, H);
    }

    // Targeted bug check: percentage padding-top/bottom on a COLUMN flex container.
    // Same rule as margin: CSS always resolves against width.
    void Fixture_padding_percent_reference_axis(float &W, float &H) {
        W = 400; H = 120;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
        root->GetStyle().Modify<PaddingEdge>().Top = CSSValue(10.0f, CSSUnit::Percent);
        root->GetStyle().Modify<PaddingEdge>().Left = CSSValue(5.0f, CSSUnit::Percent);

        auto c0 = N("c0");
        c0->GetStyle().Modify<Dimensions>().Width = 50.0f;
        c0->GetStyle().Modify<Dimensions>().Height = 30.0f;
        root->AddChild(c0);

        root->Calculate(W, H);
    }

    // Targeted bug check: position:relative + top/left offset. Style::Modify<T>() has
    // no way to reach the PositionOffsets the relative-position code path reads
    // (Style.h's enable_if list omits PositionOffsets); Dimensions.Top/Left (the field
    // that IS publicly settable) is never consulted by that code path. Expect the node
    // to NOT move at all, while a browser shifts it by (10,15).
    void Fixture_relative_position_offset(float &W, float &H) {
        W = 200; H = 200;
        auto root = N("root", OuterDisplay::Block);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;

        auto c0 = N("c0");
        c0->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
        c0->GetStyle().Modify<Dimensions>().Width = 50.0f;
        c0->GetStyle().Modify<Dimensions>().Height = 30.0f;
        c0->GetStyle().Modify<Dimensions>().Top = 15.0f;
        c0->GetStyle().Modify<Dimensions>().Left = 10.0f;
        root->AddChild(c0);

        root->Calculate(W, H);
    }

    void Fixture_absolute_basic_offsets(float &W, float &H) {
        W = 200; H = 200;
        auto root = N("root", OuterDisplay::Block);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;

        auto a = N("a"); // left/top anchored
        a->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
        a->GetStyle().Modify<Dimensions>().Width = 40.0f;
        a->GetStyle().Modify<Dimensions>().Height = 40.0f;
        a->GetStyle().Modify<Dimensions>().Left = 20.0f;
        a->GetStyle().Modify<Dimensions>().Top = 10.0f;
        root->AddChild(a);

        auto b = N("b"); // right/bottom anchored
        b->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
        b->GetStyle().Modify<Dimensions>().Width = 30.0f;
        b->GetStyle().Modify<Dimensions>().Height = 30.0f;
        b->GetStyle().Modify<Dimensions>().Right = 15.0f;
        b->GetStyle().Modify<Dimensions>().Bottom = 5.0f;
        root->AddChild(b);

        root->Calculate(W, H);
    }

    // Targeted check: an absolutely positioned box with AUTO width/height pinned by BOTH insets
    // on each axis. Per CSS 10.6.4 / 10.3.7 the auto size resolves to fill the gap between the
    // insets rather than shrinking to content — the case tests/AbsolutePositionTests.cpp's
    // absolute_layout_start_top_end_bottom asserts, and where ApplyBlockAutoHeight currently
    // overwrites the pinned height with the (zero) content height.
    void Fixture_absolute_pinned_both_insets(float &W, float &H) {
        W = 200; H = 200;
        auto root = N("root", OuterDisplay::Block);
        root->GetStyle().Modify<Dimensions>().Position = PositionType::Relative;
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;

        auto pinned = N("pinned");
        pinned->GetStyle().Modify<Dimensions>().Position = PositionType::Absolute;
        pinned->GetStyle().Modify<Dimensions>().Left = 20.0f;
        pinned->GetStyle().Modify<Dimensions>().Top = 20.0f;
        pinned->GetStyle().Modify<Dimensions>().Right = 20.0f;
        pinned->GetStyle().Modify<Dimensions>().Bottom = 20.0f;
        root->AddChild(pinned);

        root->Calculate(W, H);
    }

    void Fixture_box_model_border_padding_margin(float &W, float &H) {
        W = 300; H = 150;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        root->GetStyle().Modify<PaddingEdge>().Left = 10.0f;
        root->GetStyle().Modify<PaddingEdge>().Top = 10.0f;
        root->GetStyle().Modify<PaddingEdge>().Right = 10.0f;
        root->GetStyle().Modify<PaddingEdge>().Bottom = 10.0f;
        root->GetStyle().Modify<BorderProperties>().WidthLeft = 5.0f;
        root->GetStyle().Modify<BorderProperties>().WidthTop = 5.0f;
        root->GetStyle().Modify<BorderProperties>().WidthRight = 5.0f;
        root->GetStyle().Modify<BorderProperties>().WidthBottom = 5.0f;

        auto c0 = N("c0"); // border-box width includes its own border+padding
        c0->GetStyle().Modify<Dimensions>().Width = 100.0f;
        c0->GetStyle().Modify<Dimensions>().Height = 60.0f;
        c0->GetStyle().Modify<PaddingEdge>().Left = 8.0f;
        c0->GetStyle().Modify<BorderProperties>().WidthLeft = 3.0f;
        c0->GetStyle().Modify<MarginEdge>().Right = 12.0f;
        root->AddChild(c0);

        auto c1 = N("c1");
        c1->GetStyle().Modify<Dimensions>().Width = 60.0f;
        c1->GetStyle().Modify<Dimensions>().Height = 60.0f;
        root->AddChild(c1);

        root->Calculate(W, H);
    }

    // Targeted bug check: block-level child margin-left in NormalFlowStrategy. LayoutLine's
    // block/flex branch sets childLayout.LocalX = containerPadding.Left + containerBorder.WidthLeft
    // only -- it never adds the child's own margin-left, even though ComputeDimensions already
    // shrank the child's AUTO width by that same margin. Expect the reserved space to silently
    // land on the wrong side: Masharif's child starts flush at the content edge (x=0) instead of
    // shifted right by margin-left, while its width is still content-edge-to-content-edge minus
    // both margins (so it stops short before the container's right edge either way).
    void Fixture_block_margin_left_offset(float &W, float &H) {
        W = 200; H = 100;
        auto root = N("root", OuterDisplay::Block);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;

        auto c0 = N("c0", OuterDisplay::Block);
        c0->GetStyle().Modify<Dimensions>().Height = 40.0f;
        c0->GetStyle().Modify<MarginEdge>().Left = 30.0f;
        c0->GetStyle().Modify<MarginEdge>().Right = 10.0f;
        root->AddChild(c0);

        root->Calculate(W, H);
    }

    void Fixture_flex_basis_percent_and_auto(float &W, float &H) {
        W = 300; H = 60;
        auto root = N("root", OuterDisplay::Flex);
        root->GetStyle().Modify<Dimensions>().Width = W;
        root->GetStyle().Modify<Dimensions>().Height = H;
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;

        auto a = N("a"); // flex-basis: 50%
        a->GetStyle().Modify<CSSFlex>().FlexBasis = CSSValue(50.0f, CSSUnit::Percent);
        root->AddChild(a);

        auto b = N("b"); // flex-basis: auto, explicit width acts as basis
        b->GetStyle().Modify<Dimensions>().Width = 60.0f;
        root->AddChild(b);

        auto c = N("c"); // flex-basis: 40px, flex-grow: 1 to absorb remainder
        c->GetStyle().Modify<CSSFlex>().FlexBasis = 40.0f;
        c->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
        root->AddChild(c);

        root->Calculate(W, H);
    }
}

int main() {
    using FixtureFn = std::function<void(float &, float &)>;
    const std::vector<std::pair<std::string, FixtureFn>> fixtures = {
        {"flex_row_justify_between", Fixture_flex_row_justify_between},
        {"flex_wrap_align_content", Fixture_flex_wrap_align_content},
        {"flex_grow_shrink_minmax_clamp", Fixture_flex_grow_shrink_minmax_clamp},
        {"flex_column_percent_width_height", Fixture_flex_column_percent_width_height},
        {"align_items_variants", Fixture_align_items_variants},
        {"align_items_baseline", Fixture_align_items_baseline},
        {"order_reorder", Fixture_order_reorder},
        {"gap_row_and_column", Fixture_gap_row_and_column},
        {"margin_percent_reference_axis", Fixture_margin_percent_reference_axis},
        {"padding_percent_reference_axis", Fixture_padding_percent_reference_axis},
        {"relative_position_offset", Fixture_relative_position_offset},
        {"absolute_basic_offsets", Fixture_absolute_basic_offsets},
        {"box_model_border_padding_margin", Fixture_box_model_border_padding_margin},
        {"flex_basis_percent_and_auto", Fixture_flex_basis_percent_and_auto},
        {"block_margin_left_offset", Fixture_block_margin_left_offset},
        {"absolute_pinned_both_insets", Fixture_absolute_pinned_both_insets},
    };

    std::cout << "{";
    bool first = true;
    for (auto &[name, fn] : fixtures) {
        float w = 0, h = 0;
        fn(w, h);
        if (!first) std::cout << ",";
        first = false;
        PrintFixture(name);
    }
    std::cout << "}" << std::endl;
    return 0;
}
