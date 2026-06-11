#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include "masharifcore/Masharif.h"

using namespace masharif;

namespace {
    std::uint64_t totalStrategyRuns(const SharedNode &node) {
        std::uint64_t sum = node->GetLayout().StrategyRuns;
        for (const auto &child: node->Children())
            sum += totalStrategyRuns(child);
        return sum;
    }

    long long microsSince(const std::chrono::high_resolution_clock::time_point start) {
        const auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    SharedNode flexBox(const FlexDirection direction) {
        auto node = std::make_shared<Node>(OuterDisplay::Flex);
        node->GetStyle().Modify<CSSFlex>().Direction = direction;
        return node;
    }

    SharedNode fixedLeaf(const float width, const float height) {
        auto leaf = std::make_shared<Node>(OuterDisplay::Flex);
        leaf->GetStyle().Modify<Dimensions>().Width = width;
        leaf->GetStyle().Modify<Dimensions>().Height = height;
        return leaf;
    }
}

TEST(BenchmarkTests, MassiveLayoutBenchmark) {
    // 1. Create a massive layout
    auto root = std::make_shared<Node>();
    root->SetDisplay(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<Dimensions>().Width = 1000.0f;
    root->GetStyle().Modify<Dimensions>().Height = 1000.0f;

    const int NUM_CONTAINERS = 100;
    const int NUM_ITEMS_PER_CONTAINER = 100;
    const std::uint64_t totalNodes = 1 + NUM_CONTAINERS + NUM_CONTAINERS * NUM_ITEMS_PER_CONTAINER;
    std::vector<SharedNode> all_items;

    for (int i = 0; i < NUM_CONTAINERS; ++i) {
        auto container = std::make_shared<Node>();
        container->SetDisplay(OuterDisplay::Flex);
        container->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        container->GetStyle().Modify<CSSFlex>().Wrap = FlexWrap::Wrap;
        container->GetStyle().Modify<Dimensions>().Width = 1000.0f;
        // height auto

        for (int j = 0; j < NUM_ITEMS_PER_CONTAINER; ++j) {
            auto item = std::make_shared<Node>();
            item->GetStyle().Modify<Dimensions>().Width = 10.0f;
            item->GetStyle().Modify<Dimensions>().Height = 10.0f;
            container->AddChild(item);
            all_items.push_back(item);
        }
        root->AddChild(container);
    }

    // 2. Measure initial layout time
    auto start_initial = std::chrono::high_resolution_clock::now();
    root->Calculate(1000.0f, 1000.0f);
    const auto duration_initial = microsSince(start_initial);

    std::cout << "[BENCHMARK] Initial layout (" << totalNodes << " nodes): "
              << duration_initial << " us (" << (duration_initial / 1000.0) << " ms)" << std::endl;

    // Linearity proof: a full solve is bounded by a few strategy runs per node, never
    // the former 2^depth blowup.
    EXPECT_LE(totalStrategyRuns(root), 4 * totalNodes);

    // 3. Edit one child and recalculate. modify() propagates dirtiness to the root by
    // itself — no manual markDirtyToRoot — so this edit must actually apply.
    {
        auto &item = all_items[0];
        item->GetStyle().Modify<Dimensions>().Width = 20.0f;

        auto start_edit1 = std::chrono::high_resolution_clock::now();
        root->Calculate(1000.0f, 1000.0f);
        const auto duration_edit1 = microsSince(start_edit1);

        std::cout << "[BENCHMARK] Layout after editing 1 child: "
                  << duration_edit1 << " us (" << (duration_edit1 / 1000.0) << " ms)" << std::endl;

        EXPECT_FLOAT_EQ(20.0f, all_items[0]->GetLayout().ComputedWidth)
            << "deep style edit was silently ignored";
    }

    // 4. Edit two children and recalculate
    {
        all_items[1]->GetStyle().Modify<Dimensions>().Width = 20.0f;
        all_items[2]->GetStyle().Modify<Dimensions>().Height = 20.0f;

        auto start_edit2 = std::chrono::high_resolution_clock::now();
        root->Calculate(1000.0f, 1000.0f);
        const auto duration_edit2 = microsSince(start_edit2);

        std::cout << "[BENCHMARK] Layout after editing 2 children: "
                  << duration_edit2 << " us (" << (duration_edit2 / 1000.0) << " ms)" << std::endl;

        EXPECT_FLOAT_EQ(20.0f, all_items[1]->GetLayout().ComputedWidth);
        EXPECT_FLOAT_EQ(20.0f, all_items[2]->GetLayout().ComputedHeight);
    }

    // Across all three solves the total work must stay linear-ish in node count.
    EXPECT_LE(totalStrategyRuns(root), 6 * totalNodes);
}

/// Pre-fix, a dirty chain of nested flex containers was solved twice per level
/// (basis + definite pass), i.e. ~2^depth strategy runs for the deepest node. With
/// explicit sizes every repeat call is a cache hit, so the bound is a small constant
/// per node.
TEST(BenchmarkTests, DeepExplicitTreeIsLinearNotExponential) {
    constexpr int Depth = 20;
    auto root = flexBox(FlexDirection::Column);
    root->GetStyle().Modify<Dimensions>().Width = 1000.0f;
    root->GetStyle().Modify<Dimensions>().Height = 1000.0f;

    Node *cursor = root.get();
    for (int i = 0; i < Depth; ++i) {
        cursor->AddChild(fixedLeaf(10.0f, 10.0f));
        auto next = flexBox(i % 2 ? FlexDirection::Row : FlexDirection::Column);
        next->GetStyle().Modify<Dimensions>().Width = static_cast<float>(1000 - i * 10);
        next->GetStyle().Modify<Dimensions>().Height = static_cast<float>(1000 - i * 10);
        cursor->AddChild(next);
        cursor = next.get();
    }
    const std::uint64_t totalNodes = 2 * Depth + 1;

    const auto start = std::chrono::high_resolution_clock::now();
    root->Calculate(1000.0f, 1000.0f);
    const auto us = microsSince(start);
    std::cout << "[BENCHMARK] depth-" << Depth << " explicit chain initial layout: "
              << us << " us, strategy runs: " << totalStrategyRuns(root) << std::endl;

    EXPECT_LE(totalStrategyRuns(root), 4 * totalNodes)
        << "repeat solves of identical inputs must be cache hits";
}

/// Grow chains genuinely need re-distribution when a container's definite size differs
/// from its measured size, so the per-node constant is larger — but it must stay a
/// constant, not double per level (2^19 runs would be ~26000x this bound's slack).
TEST(BenchmarkTests, DeepGrowChainIsNotExponential) {
    constexpr int Depth = 20;
    auto root = flexBox(FlexDirection::Column);
    root->GetStyle().Modify<Dimensions>().Width = 1000.0f;
    root->GetStyle().Modify<Dimensions>().Height = 1000.0f;

    Node *cursor = root.get();
    SharedNode deepLeaf;
    for (int i = 0; i < Depth; ++i) {
        deepLeaf = fixedLeaf(50.0f, 10.0f);
        cursor->AddChild(deepLeaf);
        auto next = flexBox(i % 2 ? FlexDirection::Row : FlexDirection::Column);
        next->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f; // AUTO size + grow: NaN measure then definite re-distribution
        cursor->AddChild(next);
        cursor = next.get();
    }
    const std::uint64_t totalNodes = 2 * Depth + 1;

    const auto start = std::chrono::high_resolution_clock::now();
    root->Calculate(1000.0f, 1000.0f);
    const auto us = microsSince(start);
    std::cout << "[BENCHMARK] depth-" << Depth << " grow chain initial layout: "
              << us << " us, strategy runs: " << totalStrategyRuns(root) << std::endl;

    EXPECT_LE(totalStrategyRuns(root), 64 * totalNodes);
    EXPECT_FLOAT_EQ(50.0f, deepLeaf->GetLayout().ComputedWidth) << "tree was not actually solved";
}

TEST(BenchmarkTests, SingleLeafEditRelayoutsOnlyTheDirtyPath) {
    constexpr int Depth = 20;
    auto root = flexBox(FlexDirection::Column);
    root->GetStyle().Modify<Dimensions>().Width = 1000.0f;
    root->GetStyle().Modify<Dimensions>().Height = 1000.0f;

    Node *cursor = root.get();
    SharedNode deepLeaf;
    for (int i = 0; i < Depth; ++i) {
        deepLeaf = fixedLeaf(50.0f, 10.0f);
        cursor->AddChild(deepLeaf);
        auto next = flexBox(i % 2 ? FlexDirection::Row : FlexDirection::Column);
        next->GetStyle().Modify<CSSFlex>().FlexGrow = 1.0f;
        cursor->AddChild(next);
        cursor = next.get();
    }
    const std::uint64_t totalNodes = 2 * Depth + 1;
    root->Calculate(1000.0f, 1000.0f);
    const std::uint64_t runsAfterInitial = totalStrategyRuns(root);

    // modify() alone must both propagate to the root and bound the re-solve to the
    // dirty path plus the re-distributions it genuinely causes.
    deepLeaf->GetStyle().Modify<Dimensions>().Width = 60.0f;

    const auto start = std::chrono::high_resolution_clock::now();
    root->Calculate(1000.0f, 1000.0f);
    const auto us = microsSince(start);
    const std::uint64_t editRuns = totalStrategyRuns(root) - runsAfterInitial;
    std::cout << "[BENCHMARK] depth-" << Depth << " single leaf edit: " << us
              << " us, strategy runs: " << editRuns << std::endl;

    EXPECT_FLOAT_EQ(60.0f, deepLeaf->GetLayout().ComputedWidth)
        << "deep style edit was silently ignored";
    EXPECT_LE(editRuns, 64 * totalNodes);
}

TEST(BenchmarkTests, IdleRecalculateIsNearZeroWork) {
    auto root = std::make_shared<Node>(OuterDisplay::Flex);
    root->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Column;
    root->GetStyle().Modify<Dimensions>().Width = 1000.0f;
    root->GetStyle().Modify<Dimensions>().Height = 1000.0f;

    for (int i = 0; i < 100; ++i) {
        auto container = std::make_shared<Node>(OuterDisplay::Flex);
        container->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::Row;
        container->GetStyle().Modify<CSSFlex>().Wrap = FlexWrap::Wrap;
        container->GetStyle().Modify<Dimensions>().Width = 1000.0f;
        for (int j = 0; j < 100; ++j)
            container->AddChild(fixedLeaf(10.0f, 10.0f));
        root->AddChild(container);
    }

    root->Calculate(1000.0f, 1000.0f);
    const std::uint64_t runsAfterInitial = totalStrategyRuns(root);

    const auto start = std::chrono::high_resolution_clock::now();
    root->Calculate(1000.0f, 1000.0f); // no edits at all
    const auto us = microsSince(start);

    std::cout << "[BENCHMARK] idle recalculate (10101 nodes): " << us << " us" << std::endl;
    EXPECT_EQ(runsAfterInitial, totalStrategyRuns(root))
        << "an idle frame must not run any layout strategy";
}
