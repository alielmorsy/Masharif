#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

#include <limits>
#include <memory>

using namespace masharif;

namespace {
    constexpr float kNan = std::numeric_limits<float>::quiet_NaN();

    /// A leaf of a definite size, so a parent measuring AUTO has real content to measure.
    std::shared_ptr<Node> Leaf(const float width, const float height) {
        auto node = std::make_shared<Node>();
        node->SetDisplay(OuterDisplay::Flex);
        node->GetStyle().Modify<Dimensions>().Width = CSSValue(width);
        node->GetStyle().Modify<Dimensions>().Height = CSSValue(height);
        return node;
    }

    /// A row whose own height is AUTO (so it measures to its content) but which declares a
    /// min-height taller than that content -- the property-grid row shape.
    std::shared_ptr<Node> MinHeightRow(const float contentHeight, const float minHeight) {
        auto row = std::make_shared<Node>();
        row->SetDisplay(OuterDisplay::Flex);
        row->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        row->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f, CSSUnit::Percent);
        row->GetStyle().Modify<Dimensions>().MinHeight = CSSValue(minHeight);
        row->AddChild(Leaf(40.0f, contentHeight));
        return row;
    }

    std::shared_ptr<Node> AutoHeightColumn() {
        auto column = std::make_shared<Node>();
        column->SetDisplay(OuterDisplay::Flex);
        column->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
        column->GetStyle().Modify<CSSFlex>().Align = AlignItems::Stretch;
        return column;
    }
}

/// A shrink-to-fit column must enclose its children's HYPOTHETICAL main sizes -- the flex basis
/// after each item's own min/max clamp. Summing unclamped bases leaves the column shorter than the
/// rows it then positions, and everything after it in the parent lands on top of those rows.
TEST(MinMaxHypotheticalTests, auto_height_column_encloses_min_height_rows) {
    auto root = AutoHeightColumn();
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(200.0f);

    auto row0 = MinHeightRow(19.0f, 22.0f);
    auto row1 = MinHeightRow(19.0f, 22.0f);
    auto row2 = MinHeightRow(19.0f, 22.0f);
    root->AddChild(row0);
    root->AddChild(row1);
    root->AddChild(row2);

    root->Calculate(200.0f, kNan);

    ASSERT_FLOAT_EQ(22.0f, row0->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(22.0f, row1->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(22.0f, row2->GetLayout().ComputedHeight);

    ASSERT_FLOAT_EQ(0.0f, row0->GetLayout().LocalY);
    ASSERT_FLOAT_EQ(22.0f, row1->GetLayout().LocalY);
    ASSERT_FLOAT_EQ(44.0f, row2->GetLayout().LocalY);

    ASSERT_FLOAT_EQ(66.0f, root->GetLayout().ComputedHeight);
}

/// The inspector card: a header band, a body of min-height rows, and a 1px hairline after the body.
/// The hairline must sit below the body's last row, not across it.
TEST(MinMaxHypotheticalTests, hairline_after_min_height_body_does_not_overlap_it) {
    auto card = AutoHeightColumn();
    card->GetStyle().Modify<Dimensions>().Width = CSSValue(200.0f);

    auto header = Leaf(200.0f, 26.0f);
    auto body = AutoHeightColumn();
    for (int i = 0; i < 8; ++i) body->AddChild(MinHeightRow(19.0f, 22.0f));

    auto hairline = std::make_shared<Node>();
    hairline->SetDisplay(OuterDisplay::Flex);
    hairline->GetStyle().Modify<Dimensions>().Height = CSSValue(1.0f);

    card->AddChild(header);
    card->AddChild(body);
    card->AddChild(hairline);

    card->Calculate(200.0f, kNan);

    const float bodyBottom = body->GetLayout().LocalY + body->GetLayout().ComputedHeight;

    ASSERT_FLOAT_EQ(8 * 22.0f, body->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(bodyBottom, hairline->GetLayout().LocalY);
    ASSERT_FLOAT_EQ(26.0f + 8 * 22.0f + 1.0f, card->GetLayout().ComputedHeight);
}

/// Same defect, measured through the gap and padding channels: both are folded into the content
/// total, so a clamped basis has to be too, or the column undercounts by the clamp deltas alone.
TEST(MinMaxHypotheticalTests, auto_height_column_with_gap_and_padding_encloses_clamped_rows) {
    auto root = AutoHeightColumn();
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(200.0f);
    root->GetStyle().Modify<CSSFlex>().Gaps.Row = CSSValue(6.0f);
    root->GetStyle().Modify<PaddingEdge>().Top = CSSValue(4.0f);
    root->GetStyle().Modify<PaddingEdge>().Bottom = CSSValue(8.0f);

    root->AddChild(MinHeightRow(19.0f, 22.0f));
    root->AddChild(MinHeightRow(19.0f, 22.0f));

    root->Calculate(200.0f, kNan);

    ASSERT_FLOAT_EQ(4.0f + 22.0f + 6.0f + 22.0f + 8.0f, root->GetLayout().ComputedHeight);
}

/// A max-height on an AUTO-height item caps the hypothetical main size, so the column shrink-wraps
/// to the capped total rather than to the taller content the item measured.
TEST(MinMaxHypotheticalTests, auto_height_column_respects_child_max_height) {
    auto root = AutoHeightColumn();
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(200.0f);

    auto capped = AutoHeightColumn();
    capped->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f, CSSUnit::Percent);
    capped->GetStyle().Modify<Dimensions>().MaxHeight = CSSValue(20.0f);
    capped->AddChild(Leaf(40.0f, 60.0f));
    root->AddChild(capped);

    root->Calculate(200.0f, kNan);

    ASSERT_FLOAT_EQ(20.0f, capped->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(20.0f, root->GetLayout().ComputedHeight);
}

/// The cross axis has no other clamp site: an item's own ComputeDimensions skips min/max for an
/// AUTO size, and the basis measure runs with ignoreMinMax. A `min-height` on a ROW's child was
/// parsed and then dropped entirely.
TEST(MinMaxHypotheticalTests, cross_axis_min_height_is_honoured) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(200.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f);

    auto cell = AutoHeightColumn();
    cell->GetStyle().Modify<Dimensions>().Width = CSSValue(60.0f);
    cell->GetStyle().Modify<Dimensions>().MinHeight = CSSValue(19.0f);
    cell->AddChild(Leaf(20.0f, 10.0f));
    root->AddChild(cell);

    root->Calculate(200.0f, 100.0f);

    ASSERT_FLOAT_EQ(19.0f, cell->GetLayout().ComputedHeight);
}

/// The clamp feeds the container's content total ONLY. It must never become the freeze loop's
/// starting point, or an item sitting at its min floor and carrying a grow factor grows on top of
/// that floor: 60 + half of the remaining 40 = 80, instead of the 60/40 CSS resolves.
TEST(MinMaxHypotheticalTests, grow_item_does_not_grow_on_top_of_its_min_floor) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f);

    auto floored = std::make_shared<Node>();
    floored->SetDisplay(OuterDisplay::Flex);
    floored->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    floored->GetStyle().Modify<Dimensions>().MinWidth = CSSValue(60.0f);
    root->AddChild(floored);

    auto plain = std::make_shared<Node>();
    plain->SetDisplay(OuterDisplay::Flex);
    plain->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root->AddChild(plain);

    root->Calculate(100.0f, 100.0f);

    ASSERT_FLOAT_EQ(60.0f, floored->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(40.0f, plain->GetLayout().ComputedWidth);
}

/// Same rule on the shrink side: the scaled shrink factor is weighted by the TRUE flex base size, so
/// two items that differ only by a max constraint that never binds shrink identically. Weighting by
/// the clamped size instead splits them 56.25/43.75.
TEST(MinMaxHypotheticalTests, max_width_does_not_skew_the_shrink_ratio) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(100.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f);

    auto plain = Leaf(90.0f, 10.0f);
    plain->GetStyle().Modify<CSSFlex>().FlexShrink = 1.0f;
    root->AddChild(plain);

    auto capped = Leaf(90.0f, 10.0f);
    capped->GetStyle().Modify<CSSFlex>().FlexShrink = 1.0f;
    capped->GetStyle().Modify<Dimensions>().MaxWidth = CSSValue(70.0f);
    root->AddChild(capped);

    root->Calculate(100.0f, 100.0f);

    ASSERT_FLOAT_EQ(50.0f, plain->GetLayout().ComputedWidth);
    ASSERT_FLOAT_EQ(50.0f, capped->GetLayout().ComputedWidth);
}

/// The clamp must not steal space from a grow item: a definite column with one min-height row and
/// one grow filler still ends exactly full, with the filler absorbing the remainder.
TEST(MinMaxHypotheticalTests, min_height_row_and_grow_filler_fill_a_definite_column) {
    auto root = AutoHeightColumn();
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(200.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(100.0f);

    auto row = MinHeightRow(19.0f, 22.0f);
    auto filler = AutoHeightColumn();
    filler->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
    root->AddChild(row);
    root->AddChild(filler);

    root->Calculate(200.0f, 100.0f);

    ASSERT_FLOAT_EQ(22.0f, row->GetLayout().ComputedHeight);
    ASSERT_FLOAT_EQ(22.0f, filler->GetLayout().LocalY);
    ASSERT_FLOAT_EQ(78.0f, filler->GetLayout().ComputedHeight);
}

/// Flexbox 9.4 step 11: the cross size a stretch produces is clamped by the item's OWN used min
/// and max cross size. Nothing else clamps it -- the item's ComputeDimensions skips the clamp for
/// an AUTO size (there is no number to clamp yet), so an unclamped stretch escapes max-height
/// entirely and fills the line.
TEST(MinMaxHypotheticalTests, row_stretch_cross_size_is_clamped_by_max_height) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(300.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(56.0f);

    auto capped = std::make_shared<Node>();
    capped->SetDisplay(OuterDisplay::Flex);
    capped->GetStyle().Modify<Dimensions>().Width = CSSValue(200.0f);
    capped->GetStyle().Modify<Dimensions>().MaxHeight = CSSValue(30.0f);
    capped->GetStyle().Modify<CSSFlex>().AlignSelf = AlignItems::Stretch;
    root->AddChild(capped);

    root->Calculate(300.0f, 56.0f);

    EXPECT_FLOAT_EQ(30.0f, capped->GetLayout().ComputedHeight);
    // Clamped short of the line, a stretched item stays on the line's start edge.
    EXPECT_FLOAT_EQ(0.0f, capped->GetLayout().ComputedY);
}

/// The same clamp raises a stretch that lands under the item's min cross size, which overflows
/// the line on purpose -- min wins over max and over the line height, per the usual clamp order.
TEST(MinMaxHypotheticalTests, row_stretch_cross_size_is_raised_by_min_height) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(300.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(56.0f);

    auto raised = std::make_shared<Node>();
    raised->SetDisplay(OuterDisplay::Flex);
    raised->GetStyle().Modify<Dimensions>().Width = CSSValue(200.0f);
    raised->GetStyle().Modify<Dimensions>().MinHeight = CSSValue(80.0f);
    raised->GetStyle().Modify<CSSFlex>().AlignSelf = AlignItems::Stretch;
    root->AddChild(raised);

    root->Calculate(300.0f, 56.0f);

    EXPECT_FLOAT_EQ(80.0f, raised->GetLayout().ComputedHeight);
}

/// Symmetric case: in a column container the cross axis is horizontal, so a stretch is clamped by
/// the item's max-WIDTH.
TEST(MinMaxHypotheticalTests, column_stretch_cross_size_is_clamped_by_max_width) {
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(300.0f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(200.0f);

    auto capped = std::make_shared<Node>();
    capped->SetDisplay(OuterDisplay::Flex);
    capped->GetStyle().Modify<Dimensions>().Height = CSSValue(40.0f);
    capped->GetStyle().Modify<Dimensions>().MaxWidth = CSSValue(120.0f);
    capped->GetStyle().Modify<CSSFlex>().AlignSelf = AlignItems::Stretch;
    root->AddChild(capped);

    root->Calculate(300.0f, 200.0f);

    EXPECT_FLOAT_EQ(120.0f, capped->GetLayout().ComputedWidth);
    EXPECT_FLOAT_EQ(0.0f, capped->GetLayout().ComputedX);
}
