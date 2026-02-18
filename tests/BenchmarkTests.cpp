#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <iostream>
#include "masharifcore/Masharif.h"

using namespace masharif;

TEST(BenchmarkTests, MassiveLayoutBenchmark) {
    // 1. Create a massive layout
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;
    root->style().modify<Dimensions>().width = 1000.0f;
    root->style().modify<Dimensions>().height = 1000.0f;

    const int NUM_CONTAINERS = 100;
    const int NUM_ITEMS_PER_CONTAINER = 100;
    std::vector<std::shared_ptr<Node>> all_items;

    // Create 100 containers, each with 100 items -> 10,000 items + 100 containers + 1 root = 10,101 nodes
    for (int i = 0; i < NUM_CONTAINERS; ++i) {
        auto container = std::make_shared<Node>();
        container->setDisplay(OuterDisplay::Flex);
        container->style().modify<CSSFlex>().direction = FlexDirection::Row;
        container->style().modify<CSSFlex>().wrap = FlexWrap::Wrap;
        container->style().modify<Dimensions>().width = 1000.0f;
        // height auto
        
        for (int j = 0; j < NUM_ITEMS_PER_CONTAINER; ++j) {
            auto item = std::make_shared<Node>();
            item->style().modify<Dimensions>().width = 10.0f;
            item->style().modify<Dimensions>().height = 10.0f;
            container->addChild(item);
            all_items.push_back(item);
        }
        root->addChild(container);
    }

    // 2. Measure initial layout time
    auto start_initial = std::chrono::high_resolution_clock::now();
    root->calculate(1000.0f, 1000.0f);
    auto end_initial = std::chrono::high_resolution_clock::now();
    auto duration_initial = std::chrono::duration_cast<std::chrono::microseconds>(end_initial - start_initial).count();

    std::cout << "[BENCHMARK] Initial layout (" << (NUM_CONTAINERS * NUM_ITEMS_PER_CONTAINER + NUM_CONTAINERS + 1) << " nodes): " 
              << duration_initial << " us (" << (duration_initial / 1000.0) << " ms)" << std::endl;

    // 3. Edit one child and recalculate
    if (!all_items.empty()) {
        auto& item = all_items[0];
        item->style().modify<Dimensions>().width = 20.0f;
        
        // Note: verify if `calculate` does full layout or incremental. 
        // If the engine supports dirty flags, this should be faster.
        
        auto start_edit1 = std::chrono::high_resolution_clock::now();
        root->calculate(1000.0f, 1000.0f);
        auto end_edit1 = std::chrono::high_resolution_clock::now();
        auto duration_edit1 = std::chrono::duration_cast<std::chrono::microseconds>(end_edit1 - start_edit1).count();

        std::cout << "[BENCHMARK] Layout after editing 1 child: " 
                  << duration_edit1 << " us (" << (duration_edit1 / 1000.0) << " ms)" << std::endl;
    }

    // 4. Edit two children and recalculate
    if (all_items.size() >= 2) {
        auto& item1 = all_items[1];
        auto& item2 = all_items[2];
        item1->style().modify<Dimensions>().width = 20.0f;
        item2->style().modify<Dimensions>().height = 20.0f;

        auto start_edit2 = std::chrono::high_resolution_clock::now();
        root->calculate(1000.0f, 1000.0f);
        auto end_edit2 = std::chrono::high_resolution_clock::now();
        auto duration_edit2 = std::chrono::duration_cast<std::chrono::microseconds>(end_edit2 - start_edit2).count();

        std::cout << "[BENCHMARK] Layout after editing 2 children: " 
                  << duration_edit2 << " us (" << (duration_edit2 / 1000.0) << " ms)" << std::endl;
    }
}
