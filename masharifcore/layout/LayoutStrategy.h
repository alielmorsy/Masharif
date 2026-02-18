#pragma once


namespace masharif {
    class Node;

    struct LayoutStrategy {
    protected:
        Node *container;

    public:
        explicit LayoutStrategy(Node *node) : container(node) {
        }

        virtual ~LayoutStrategy() = default;


        virtual void layout(float availableWidth, float availableHeight) = 0;
    };
}
