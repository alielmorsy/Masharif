#include <iostream>
#include <vector>
#include <chrono>
#include <memory>
#include <masharifcore/Masharif.h>

using namespace _NAMESPACE;

// Helper to measure function execution time
template<typename Func>
long long measureTime(Func func, const std::string &name) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "[BENCHMARK] " << name << ": " << duration << " us (" << (duration / 1000.0) << " ms)\n";
    return duration;
}

int main() {
    std::cout << "Starting Massive Layout Benchmark..." << std::endl;

    // 1. Setup Root Node
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;
    root->style().modify<Dimensions>().width = 2000.0f; // Large width to accommodate items
    root->style().modify<Dimensions>().height = CSSValue(NAN, CSSUnit::AUTO); // Height depends on content

    const int NUM_CONTAINERS = 100;
    const int NUM_ITEMS_PER_CONTAINER = 100;
    int total_nodes = 1 + NUM_CONTAINERS + (NUM_CONTAINERS * NUM_ITEMS_PER_CONTAINER);

    std::vector<std::shared_ptr<Node> > containers;
    std::vector<std::shared_ptr<Node> > all_items;
    all_items.reserve(NUM_CONTAINERS * NUM_ITEMS_PER_CONTAINER);

    // 2. Build the Tree
    std::cout << "Building tree with " << total_nodes << " nodes..." << std::endl;

    for (int i = 0; i < NUM_CONTAINERS; ++i) {
        auto container = std::make_shared<Node>();
        container->setDisplay(OuterDisplay::Flex);
        container->style().modify<CSSFlex>().direction = FlexDirection::Row;
        container->style().modify<CSSFlex>().wrap = FlexWrap::Wrap;
        container->style().modify<Dimensions>().width = 100.0f; // Percentage would be better but using fixed for now
        // Or set to 100% via CSSValue if supported, but let's stick to float for simplicity unless we want to test % resolution

        // Add some variety to containers
        if (i % 2 == 0) {
            container->style().modify<PaddingEdge>().left = 10.0f;
            container->style().modify<PaddingEdge>().right = 10.0f;
            container->style().modify<PaddingEdge>().top = 5.0f;
            container->style().modify<PaddingEdge>().bottom = 5.0f;
        }

        for (int j = 0; j < NUM_ITEMS_PER_CONTAINER; ++j) {
            auto item = std::make_shared<Node>();

            // Base styles
            item->style().modify<Dimensions>().width = 20.0f;
            item->style().modify<Dimensions>().height = 20.0f;

            // Variety based on index
            int idx = i * NUM_ITEMS_PER_CONTAINER + j;

            // 1. Flex Grow
            if (idx % 2 == 0) {
                item->style().modify<CSSFlex>().flexGrow = 1.0f;
            }

            // 2. Flex Shrink (default is 1, let's change it)
            if (idx % 3 == 0) {
                item->style().modify<CSSFlex>().flexShrink = 0.5f;
            }

            // 3. Margin
            if (idx % 5 == 0) {
                item->style().modify<MarginEdge>().left = 2.0f;
                item->style().modify<MarginEdge>().right = 2.0f;
                item->style().modify<MarginEdge>().top = 2.0f;
                item->style().modify<MarginEdge>().bottom = 2.0f;
            }

            // 4. Padding
            if (idx % 7 == 0) {
                item->style().modify<PaddingEdge>().left = 5.0f;
            }

            // 5. Flex Basis
            if (idx % 11 == 0) {
                item->style().modify<CSSFlex>().flexBasis = 50.0f;
            }

            if (idx % 13 == 0) {
                item->style().modify<MarginEdge>().right = {};
            }
            container->addChild(item);
            all_items.push_back(item);
        }
        root->addChild(container);
        containers.push_back(container);
    }

    // 3. Initial Layout
    measureTime([&]() {
        root->calculate(2000.0f, 2000.0f); // Providing available space
    }, "Initial Layout");

    // 4. Edit one child (deep in the tree)
    if (!all_items.empty()) {
        int target_idx = all_items.size() / 2; // Middle item
        auto target = all_items[target_idx];

        std::cout << "\nModifying item " << target_idx << " width to 50.0f..." << std::endl;
        target->style().modify<Dimensions>().width = 50.0f;

        measureTime([&]() {
            root->calculate(2000.0f, 2000.0f);
        }, "Recalculate after 1 edit");
    }

    // 5. Edit two children in different containers
    if (containers.size() >= 2) {
        auto item1 = containers[0]->children[0];
        auto item2 = containers[containers.size() - 1]->children[0];

        std::cout << "\nModifying first item of first container (height=40) and last container (margin=10)..." <<
                std::endl;
        item1->style().modify<Dimensions>().height = 40.0f;
        item2->style().modify<MarginEdge>().top = 10.0f;

        measureTime([&]() {
            root->calculate(2000.0f, 2000.0f);
        }, "Recalculate after 2 edits");
    }

    // 6. Multiple edits to trigger complex layout changes
    if (!all_items.empty()) {
        std::cout << "\nModifying 100 items (flex-grow toggles)..." << std::endl;
        for (int k = 0; k < 100; ++k) {
            int idx = (k * 17) % all_items.size();
            all_items[idx]->style().modify<CSSFlex>().flexGrow = 2.0f;
        }

        measureTime([&]() {
            root->calculate(2000.0f, 2000.0f);
        }, "Recalculate after 100 edits");
    }

    // 7. Remove one child and recalculate
    if (!all_items.empty()) {
        auto item_to_remove = all_items.back();
        auto parent = item_to_remove->parent(); // Node*
        
        std::cout << "\nRemoving one item from the last container..." << std::endl;
        if(parent) {
             parent->removeChild(item_to_remove);
             all_items.pop_back(); // Remove from our tracking list too
             
             measureTime([&]() {
                root->calculate(2000.0f, 2000.0f);
            }, "Recalculate after removing 1 child");
        }
    }

    return 0;
}
