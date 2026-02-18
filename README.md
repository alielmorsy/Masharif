# Masharif (مشارف) or overlooked
An implementation for CSS Box Layout Engine

You can build example and check it till I finalize  its docs

It may not be compelete But I will really accept anyone's trying to help


### Supported Features

#### 1. Flexbox Layout
Complete support for Flexbox properties:
- **Container Properties:**
  - `flex-direction`: `row`, `row-reverse`, `column`, `column-reverse`
  - `flex-wrap`: `nowrap`, `wrap`, `wrap-reverse`
  - `justify-content`: `flex-start`, `flex-end`, `center`, `space-between`, `space-around`, `space-evenly`
  - `align-items`: `flex-start`, `flex-end`, `center`, `baseline`, `stretch`
  - `align-content`: `flex-start`, `flex-end`, `center`, `space-between`, `space-around`, `stretch`
  - `gap`: Row and Column gaps

- **Item Properties:**
  - `align-self`: Overrides `align-items` for individual items
  - `flex-grow`: Defines the ability for a flex item to grow
  - `flex-shrink`: Defines the ability for a flex item to shrink
  - `flex-basis`: Defines the default size of an element before the remaining space is distributed
  - `order`: Controls the order in which flex items appear

#### 2. Box Model & Dimensions
- **Dimensions:**
  - `width`, `height`
  - `min-width`, `min-height`
  - `max-width`, `max-height`
- **Spacing:**
  - `margin`: Top, Bottom, Left, Right
  - `padding`: Top, Bottom, Left, Right
- **Borders:**
  - `border-width`: Support for individual side widths
  - `border-radius`: Support for individual corner radii

#### 3. Positioning
- **Position Types:** `static`, `relative`, `absolute`
- **Offsets:** `top`, `bottom`, `left`, `right`

#### 4. Unit System
- **Pixels (PX)**: Absolute values
- **Percentage (%)**: Relative to parent dimensions
- **Auto**: Automatic sizing based on content or context

#### 5. Performance
- **Incremental Layout**: Smart dirty checking to avoid unnecessary recalculations for unchanged subtrees.


# Benchmark
```
Building tree with 10101 nodes...
[BENCHMARK] Initial Layout: 650 us (0.65 ms)

Modifying item 5000 width to 50.0f...
[BENCHMARK] Recalculate after 1 edit: 78 us (0.078 ms)

Modifying first item of first container (height=40) and last container (margin=10)...
[BENCHMARK] Recalculate after 2 edits: 56 us (0.056 ms)

Modifying 100 items (flex-grow toggles)...
[BENCHMARK] Recalculate after 100 edits: 75 us (0.075 ms)

Removing one item from the last container...
[BENCHMARK] Recalculate after removing 1 child: 600 us (0.6 ms)


```
