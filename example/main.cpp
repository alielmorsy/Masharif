#include <chrono>
#include <iostream>
#include <ostream>
#include <masharifcore/Masharif.h>


#define WIDTH 1920
#define HEIGHT 1080
using namespace _NAMESPACE;

void printNode(Node *node, int depth = 0) {
    std::string indent(depth * 2, ' ');


    std::cout << indent << "Node at (" << node->layout().computedX << ", " << node->layout().computedY
            << "), Size: " << node->layout().computedWidth << "x" << node->layout().computedHeight
            << ", Display: " << static_cast<int>(node->style().modify<Dimensions>().display)
            << ", Dirty: (" << node->style().dirty << ")\n";
    for (const auto &child: node->children) {
        printNode(child.get(), depth + 1);
    }
}


int main() {
    // Initialize the LayoutEngine


    // Create the root node (e.g., <body>)
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<CSSFlex>().wrap=FlexWrap::Wrap;
    root->style().modify<Dimensions>().width = {800.0f, CSSUnit::PX}; // Fixed width
    root->style().modify<Dimensions>().height = {600.0f, CSSUnit::PX}; // Fixed height
    // root->style().modify<PaddingEdge>().top = {10.0f, CSSUnit::PX};
    auto empty = std::shared_ptr<Node>();
    // Create a container div
    auto container = std::make_shared<Node>();
    container->setDisplay(OuterDisplay::Block);
    container->style().modify<Dimensions>().width = {600.0f, CSSUnit::PX}; // Fixed width
    container->style().modify<PaddingEdge>().top = 10.0f;
    container->style().modify<PaddingEdge>().bottom = 10.0f;

    // Create a paragraph (block element)
    auto paragraph = std::make_shared<Node>();
    paragraph->setDisplay(OuterDisplay::Block);
    paragraph->style().modify<Dimensions>().height = {100.0f, CSSUnit::PX}; // Fixed height
    paragraph->style().modify<MarginEdge>().top = 5.0f;
    paragraph->style().modify<MarginEdge>().bottom = 5.0f;

    // Create a span (inline-block element)
    auto span = std::make_shared<Node>();
    span->setDisplay(OuterDisplay::InlineBlock);
    span->style().modify<Dimensions>().width = {150.0f, CSSUnit::PX};
    span->style().modify<Dimensions>().height = {20.0f, CSSUnit::PX};

    // Build the tree: root -> container -> paragraph -> span
    root->addChild(container);
    root->addChild(paragraph);
    root->addChild(span);

    // Perform initial layout
    std::cout << "Performing initial layout...\n";

    auto start = std::chrono::high_resolution_clock::now();
    root->calculate(WIDTH, HEIGHT);
    auto end = std::chrono::high_resolution_clock::now();
    auto difference=end-start;
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(difference).count();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(difference).count();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(difference).count();
    auto duration_s  = std::chrono::duration_cast<std::chrono::seconds>(difference).count();

    // Print readable time with appropriate unit
    std::cout << "Time taken: ";
    if (duration_s > 0)
        std::cout << duration_s << " s";
    else if (duration_ms > 0)
        std::cout << duration_ms << " ms";
    else if (duration_us > 0)
        std::cout << duration_us << " µs";  // µs = microseconds
    else
        std::cout << duration_ns << " ns";  // ns = nanoseconds
    std::cout << "Tree after initial layout:\n";
    printNode(root.get());

    // Simulate a user interaction: Change the span's width (e.g., typing expands it)
    std::cout << "\nModifying span width to 200px...\n";
    // span->style().modify<Dimensions>().height = {200.0f, CSSUnit::PX};


    // Perform incremental layout
    std::cout << "Performing incremental layout...\n";
    start = std::chrono::high_resolution_clock::now();
    root->calculate(WIDTH, HEIGHT);
    end = std::chrono::high_resolution_clock::now();
    difference=end-start;
    duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(difference).count();
    duration_us = std::chrono::duration_cast<std::chrono::microseconds>(difference).count();
    duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(difference).count();
    duration_s  = std::chrono::duration_cast<std::chrono::seconds>(difference).count();

    // Print readable time with appropriate unit
    std::cout << "Time taken: ";
    if (duration_s > 0)
        std::cout << duration_s << " s";
    else if (duration_ms > 0)
        std::cout << duration_ms << " ms";
    else if (duration_us > 0)
        std::cout << duration_us << " µs";  // µs = microseconds
    else
        std::cout << duration_ns << " ns";  // ns = nanoseconds
    std::cout << "Tree after incremental layout:\n";
    printNode(root.get());

    // Simulate removing the span (e.g., hiding an element)
    std::cout << "\nRemoving span...\n";
    std::cout << "Tree after removing span:\n";
    printNode(root.get());

    return 0;
}
