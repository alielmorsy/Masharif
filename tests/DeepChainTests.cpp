#include <gtest/gtest.h>
#include "masharifcore/Masharif.h"

#include <memory>

using namespace masharif;

namespace {
    long long Runs(Node& n) {
        long long s = n.GetLayout().StrategyRuns;
        for (auto& c : n.Children()) s += Runs(*c);
        return s;
    }

    /// Auto-sized padded containers nested deep: the measure cache must hold, so strategy runs
    /// stay linear in depth instead of exploding.
    void DeepPaddedChainIsLinear(const FlexDirection dir) {
        constexpr int kDepth = 256;
        auto root = std::make_shared<Node>(OuterDisplay::Flex);
        root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
        root->GetStyle().Modify<Dimensions>().Width = CSSValue(1920.f);
        root->GetStyle().Modify<Dimensions>().Height = CSSValue(1080.f);
        auto leaf = std::make_shared<Node>(OuterDisplay::Flex);
        leaf->GetStyle().Modify<Dimensions>().Width = CSSValue(40.f);
        leaf->GetStyle().Modify<Dimensions>().Height = CSSValue(40.f);
        SharedNode cur = leaf;
        for (int i = 0; i < kDepth; ++i) {
            auto b = std::make_shared<Node>(OuterDisplay::Flex);
            b->GetStyle().Modify<CSSFlex>().Direction = dir;
            auto& p = b->GetStyle().Modify<PaddingEdge>();
            p.Left = p.Top = p.Right = p.Bottom = CSSValue(1.f);
            b->AddChild(cur);
            cur = b;
        }
        root->AddChild(cur);
        root->Calculate(1920, 1080);
        EXPECT_LT(Runs(*root), 8LL * kDepth);
        EXPECT_FLOAT_EQ(leaf->GetLayout().ComputedX, float(kDepth));
        EXPECT_FLOAT_EQ(cur->GetLayout().ComputedHeight, 40.f + 2.f * kDepth);
    }

    std::shared_ptr<Node> Column() {
        auto n = std::make_shared<Node>(OuterDisplay::Flex);
        n->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
        return n;
    }

    std::shared_ptr<Node> Leaf(const CSSValue w, const float h) {
        auto n = std::make_shared<Node>(OuterDisplay::Flex);
        n->GetStyle().Modify<Dimensions>().Width = w;
        n->GetStyle().Modify<Dimensions>().Height = CSSValue(h);
        return n;
    }
}

TEST(DeepChain, PaddedColumnChainSolvesLinearly) { DeepPaddedChainIsLinear(FlexDirection::Column); }
TEST(DeepChain, PaddedRowChainSolvesLinearly) { DeepPaddedChainIsLinear(FlexDirection::Row); }

/// A centered auto-width box must not reuse last frame's shrink-wrapped width when only a
/// descendant is dirty.
TEST(DeepChain, DescendantOnlyEditDoesNotReuseStaleShrinkWrap) {
    auto root = Column();
    root->GetStyle().Modify<Dimensions>().Width = CSSValue(400.f);
    root->GetStyle().Modify<Dimensions>().Height = CSSValue(300.f);
    root->GetStyle().Modify<CSSFlex>().Align = AlignItems::FlexCenter;
    auto item = Column();
    auto& p = item->GetStyle().Modify<PaddingEdge>();
    p.Left = p.Top = p.Right = p.Bottom = CSSValue(1.f);
    root->AddChild(item);
    auto c = Leaf(CSSValue(10.f), 10);
    item->AddChild(Leaf(CSSValue(100.f), 10));
    item->AddChild(Leaf(CSSValue(50.f, CSSUnit::Percent), 10));
    item->AddChild(c);
    root->Calculate(400, 300);
    const float w = item->GetLayout().ComputedWidth;
    c->GetStyle().Modify<Dimensions>().Height = CSSValue(20.f);
    root->Calculate(400, 300);
    EXPECT_FLOAT_EQ(item->GetLayout().ComputedWidth, w);
}
