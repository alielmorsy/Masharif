#include <iostream>
#include <ostream>
#include <cmath>
#include <masharifcore/Masharif.h>

#define WIDTH 1920
#define HEIGHT 1080
using namespace _NAMESPACE;

void printNode(Node *node, int depth = 0) {
    std::string indent(depth * 2, ' ');
    std::cout << indent << "Node at (" << node->layout().computedX << ", " << node->layout().computedY
            << "), Size: " << node->layout().computedWidth << "x" << node->layout().computedHeight
            << ", FlexBasis: " << node->layout().computedFlexBasis
            << "\n";
    for (const auto &child: node->children) {
        printNode(child.get(), depth + 1);
    }
}

bool approxEqual(float a, float b, float eps = 0.5f) {
    return std::fabs(a - b) < eps;
}

#define CHECK(cond, msg)                                                   \
    do {                                                                    \
        if (!(cond)) {                                                       \
            std::cout << "  FAIL: " << msg << "\n";                          \
            passed = false;                                                  \
        }                                                                    \
    } while (0)

// ── Test 1: Basic flex grow ─────────────────────────────────────────────────
bool testBasicGrow() {
    bool passed = true;
    std::cout << "== Test 1: Basic Flex Grow ==\n";

    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = {400.0f, CSSUnit::PX};

    auto a = std::make_shared<Node>();
    a->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    a->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    a->style().modify<CSSFlex>().flexGrow = 1;
    root->addChild(a);

    auto b = std::make_shared<Node>();
    b->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    b->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    b->style().modify<CSSFlex>().flexGrow = 1;
    root->addChild(b);

    root->calculate(WIDTH, HEIGHT);
    printNode(root.get());

    // Each item starts at 100px, 200px free space split equally → 200px each
    CHECK(approxEqual(a->layout().computedFlexBasis, 200.0f), "A should grow to 200");
    CHECK(approxEqual(b->layout().computedFlexBasis, 200.0f), "B should grow to 200");
    CHECK(approxEqual(a->layout().computedX, 0.0f), "A.x should be 0");
    CHECK(approxEqual(b->layout().computedX, 200.0f), "B.x should be 200");

    std::cout << (passed ? "  PASSED\n" : "") << "\n";
    return passed;
}

// ── Test 2: Grow with max-width clamping (multi-pass) ───────────────────────
bool testGrowWithMaxWidth() {
    bool passed = true;
    std::cout << "== Test 2: Grow with Max-Width Clamping ==\n";

    // Container: 400px, items A(100px, grow=1, maxWidth=150) B(100px, grow=1)
    // Pass 1: 200px free → each gets +100 → A=200 CLAMPED to 150, B=200
    // Pass 2: A frozen@150, remaining = 400-150-100 = 150 → B gets all → 250
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = {400.0f, CSSUnit::PX};

    auto a = std::make_shared<Node>();
    a->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    a->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    a->style().modify<Dimensions>().maxWidth = {150.0f, CSSUnit::PX};
    a->style().modify<CSSFlex>().flexGrow = 1;
    root->addChild(a);

    auto b = std::make_shared<Node>();
    b->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    b->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    b->style().modify<CSSFlex>().flexGrow = 1;
    root->addChild(b);

    root->calculate(WIDTH, HEIGHT);
    printNode(root.get());

    CHECK(approxEqual(a->layout().computedFlexBasis, 150.0f), "A clamped to maxWidth 150");
    CHECK(approxEqual(b->layout().computedFlexBasis, 250.0f), "B gets remaining → 250");

    std::cout << (passed ? "  PASSED\n" : "") << "\n";
    return passed;
}

// ── Test 3: Shrink with min-width clamping (multi-pass) ─────────────────────
bool testShrinkWithMinWidth() {
    bool passed = true;
    std::cout << "== Test 3: Shrink with Min-Width Clamping ==\n";

    // Container: 400px, items A(300px, shrink=1, minWidth=250) B(300px, shrink=1)
    // Total=600, overflow=200 → each should shrink by 100 → A=200 CLAMPED to 250
    // Pass 2: A frozen@250, free=400-250=150, B basis=300, needs to fit → B=150
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = {400.0f, CSSUnit::PX};

    auto a = std::make_shared<Node>();
    a->style().modify<Dimensions>().width = {300.0f, CSSUnit::PX};
    a->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    a->style().modify<Dimensions>().minWidth = {250.0f, CSSUnit::PX};
    a->style().modify<CSSFlex>().flexShrink = 1;
    root->addChild(a);

    auto b = std::make_shared<Node>();
    b->style().modify<Dimensions>().width = {300.0f, CSSUnit::PX};
    b->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    b->style().modify<CSSFlex>().flexShrink = 1;
    root->addChild(b);

    root->calculate(WIDTH, HEIGHT);
    printNode(root.get());

    CHECK(approxEqual(a->layout().computedFlexBasis, 250.0f), "A clamped to minWidth 250");
    CHECK(approxEqual(b->layout().computedFlexBasis, 150.0f), "B absorbs remaining → 150");

    std::cout << (passed ? "  PASSED\n" : "") << "\n";
    return passed;
}

// ── Test 4: Order property ──────────────────────────────────────────────────
bool testOrderProperty() {
    bool passed = true;
    std::cout << "== Test 4: Order Property ==\n";

    // Three items added in DOM order: A, B, C
    // order: A=2, B=0, C=1 → visual order should be B, C, A
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = {300.0f, CSSUnit::PX};

    auto a = std::make_shared<Node>();
    a->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    a->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    a->style().modify<CSSFlex>().order = 2;
    root->addChild(a);

    auto b = std::make_shared<Node>();
    b->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    b->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    b->style().modify<CSSFlex>().order = 0;
    root->addChild(b);

    auto c = std::make_shared<Node>();
    c->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    c->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    c->style().modify<CSSFlex>().order = 1;
    root->addChild(c);

    root->calculate(WIDTH, HEIGHT);
    printNode(root.get());

    // Visual order: B at x=0, C at x=100, A at x=200
    CHECK(approxEqual(b->layout().computedX, 0.0f),   "B (order=0) at x=0");
    CHECK(approxEqual(c->layout().computedX, 100.0f),  "C (order=1) at x=100");
    CHECK(approxEqual(a->layout().computedX, 200.0f),  "A (order=2) at x=200");

    std::cout << (passed ? "  PASSED\n" : "") << "\n";
    return passed;
}

// ── Test 5: Padding not double-counted ──────────────────────────────────────
bool testPaddingNotDoubled() {
    bool passed = true;
    std::cout << "== Test 5: Padding Not Double-Counted ==\n";

    // Container: 400px wide, 20px padding each side → 360px inner space
    // One child with flexGrow=1 should fill 360px, positioned at x=20
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = {400.0f, CSSUnit::PX};
    root->style().modify<PaddingEdge>().left = {20.0f, CSSUnit::PX};
    root->style().modify<PaddingEdge>().right = {20.0f, CSSUnit::PX};

    auto child = std::make_shared<Node>();
    child->style().modify<Dimensions>().width = {100.0f, CSSUnit::PX};
    child->style().modify<Dimensions>().height = {50.0f, CSSUnit::PX};
    child->style().modify<CSSFlex>().flexGrow = 1;
    root->addChild(child);

    root->calculate(WIDTH, HEIGHT);
    printNode(root.get());

    CHECK(approxEqual(child->layout().computedFlexBasis, 360.0f), "Child fills 360px (400-20-20)");
    CHECK(approxEqual(child->layout().computedX, 20.0f), "Child starts at x=20 (after left padding)");

    std::cout << (passed ? "  PASSED\n" : "") << "\n";
    return passed;
}


int main() {
    auto root = std::make_shared<Node>();
    root->setDisplay(OuterDisplay::Flex);
    root->style().modify<Dimensions>().width = 100.0f;
    root->style().modify<Dimensions>().height = 100.0f;
    root->style().modify<CSSFlex>().direction = FlexDirection::Column;

    auto root_child0 = std::make_shared<Node>();
    root_child0->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child0->style().modify<CSSFlex>().flexBasis = 50.0f;
    root_child0->style().modify<Dimensions>().height = 20.0f;
    root->addChild(root_child0);

    auto root_child1 = std::make_shared<Node>();
    root_child1->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child1->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child1);

    auto root_child2 = std::make_shared<Node>();
    root_child2->style().modify<CSSFlex>().flexGrow = 1.0f;
    root_child2->style().modify<Dimensions>().height = 10.0f;
    root->addChild(root_child2);

    root->calculate(100, 100);
}
