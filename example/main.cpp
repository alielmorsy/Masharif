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
    root->style().modify<Dimensions>().width = {400.0f, CSSUnit::PX}; // Fixed width
    auto one = std::make_shared<Node>();
    one->style().modify<CSSFlex>().flexShrink = 1;
    one->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    one->style().modify<Dimensions>().height = {190.0f, CSSUnit::PX};
    root->addChild(one);

    auto two = std::make_shared<Node>();
    two->style().modify<CSSFlex>().flexGrow = 1;
    two->style().modify<Dimensions>().width = {200.0f, CSSUnit::PX};
    two->style().modify<Dimensions>().height = {190.0f, CSSUnit::PX};
    root->addChild(two);

    auto three = std::make_shared<Node>();
    three->style().modify<Dimensions>().width = {275.5f, CSSUnit::PX};
    three->style().modify<CSSFlex>().flexShrink = 10;
    root->addChild(three);
    root->calculate(WIDTH, HEIGHT);
    printNode(root.get());

    return 0;
}
