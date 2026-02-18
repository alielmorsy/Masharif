# Masharif (مشارف) or overlooked
An implementation for CSS Box Layout Engine

You can build example and check it till I finalize  its docs

It may not be compelete But I will really accept anyone's trying to help

BTW, It supports incremental layout dirty bit. So, no recalculations over unchanged variables

# Benchmark
```
Building tree with 10101 nodes...
[BENCHMARK] Initial Layout: 877 us (0.877 ms)

Modifying item 5000 width to 50.0f...
[BENCHMARK] Recalculate after 1 edit: 78 us (0.078 ms)

Modifying first item of first container (height=40) and last container (margin=10)...
[BENCHMARK] Recalculate after 2 edits: 56 us (0.056 ms)

Modifying 100 items (flex-grow toggles)...
[BENCHMARK] Recalculate after 100 edits: 75 us (0.075 ms)

Removing one item from the last container...
[BENCHMARK] Recalculate after removing 1 child: 751 us (0.751 ms)


```
