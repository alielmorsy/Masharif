// Full-page case: a long-form article laid out in normal flow.
//
// This is the page the Yoga suite in tests-yoga/ cannot express at all. Everything here goes
// through NormalFlowStrategy: a root with an AUTO height that has to be derived from its
// children, AUTO-width block children that fill the content box less their own horizontal
// margins, a flex container appearing as a block-level child mid-document, and a run of twelve
// `inline-block` chips whose line breaks, per-line height and second-line offset are all decided
// by the strategy's line loop rather than stated anywhere.
//
// Two deliberate shapes, both load-bearing:
//   * every vertical gap is a margin-BOTTOM on the box above, never a margin-top on the box
//     below (R7). The engine writes LocalY without the child's own margin-top; CSS additionally
//     collapses adjacent sibling margins, which the engine does not model. Writing the spacing
//     one-sided means the two engines are answering the same question rather than being compared
//     on one neither of them was asked.
//   * `chips` has an explicit width and no padding or border (R5). LayoutLine starts its x at the
//     border-box origin, so a padded inline container could not agree with the browser -- and an
//     AUTO width would make the wrap threshold depend on a different number on each side.

const C = {
    ink: '#1b1b1f', body: '#4a4a55', mute: '#a0a0ad', rule: '#e3e3ea',
    page: '#ffffff', tint: '#f3f4f8', accent: '#8b5cf6', warm: '#e8833a',
};

/// AUTO-width block: fills the container's content box less its own horizontal margins.
const block = (id, height, extra = {}) => ({ id, display: 'block', height, color: C.rule, ...extra });

const para = (id, height, mb) => ({
    id, display: 'block', height, margin: { bottom: mb }, color: C.rule,
});

/// Widths and heights are picked so the wrap lands mid-run at c6, and so no chip is ambiguous
/// about it: every chip either clears the 780px content width by its border box alone or misses
/// it by its border box alone, so the browser's margin-inclusive fit test and the engine's
/// `currentX + width + margins > availW` test cannot disagree about where the line breaks.
const CHIPS = [
    [96, 24], [120, 24], [72, 32], [140, 24], [88, 28], [110, 24],
    [130, 24], [64, 24], [150, 24], [100, 30], [84, 24], [118, 24],
];

const chip = (i, [w, h]) => ({
    id: `chip${i}`, display: 'inline-block', width: w, height: h,
    // margin-right advances the line cursor and margin-bottom feeds the line height; both agree
    // with CSS. margin-left/top would not (R6).
    margin: { right: 8, bottom: 8 },
    color: i % 3 === 0 ? C.accent : C.tint,
});

const relatedCard = (i) => ({
    id: `rel${i}`, display: 'flex', direction: 'column', gapRow: 10,
    flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
    padding: { top: 16, right: 16, bottom: 16, left: 16 },
    border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.page,
    children: [
        { id: `rel${i}_img`, display: 'flex', height: 80, color: C.tint },
        { id: `rel${i}_title`, display: 'flex', width: '80%', height: 14, color: C.ink },
        { id: `rel${i}_meta`, display: 'flex', width: 90, height: 10, color: C.mute },
    ],
});

export default {
    name: 'page_article_blockflow',
    title: 'Full page: long-form article in normal flow (block stack + inline-block chip run)',
    note: 'AUTO root height, AUTO-width block children, flex-inside-block, and a wrapping inline-block run.',
    // The root height is AUTO, so the available space has to be stated here rather than read off
    // the root; 1400 matches extract.mjs's viewport so both sides see the same figure.
    calc: { w: 900, h: 1400 },
    root: {
        // No padding-top and no border-top: ApplyBlockAutoHeight adds both on top of a child
        // LocalY that already includes them (R8). Horizontal and bottom padding are unaffected.
        id: 'root', display: 'block', width: 900,
        padding: { right: 60, bottom: 60, left: 60 }, color: C.page,
        children: [
            block('art_kicker', 14, { margin: { bottom: 12 }, color: C.warm }),
            block('art_title', 48, { margin: { bottom: 16 }, color: C.ink }),

            // A flex container as a block-level child: the block branch places it, its own
            // strategy lays out its children.
            {
                id: 'art_byline', display: 'flex', direction: 'row', alignItems: 'center', gapColumn: 12,
                height: 40, margin: { bottom: 24 },
                children: [
                    { id: 'by_avatar', display: 'flex', width: 40, height: 40, color: C.accent },
                    {
                        id: 'by_col', display: 'flex', direction: 'column', gapRow: 6,
                        flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
                        children: [
                            { id: 'by_name', display: 'flex', height: 12, color: C.ink },
                            { id: 'by_meta', display: 'flex', width: '40%', height: 10, color: C.mute },
                        ],
                    },
                    {
                        id: 'by_share', display: 'flex', direction: 'row', gapColumn: 8,
                        flexGrow: 0, flexShrink: 0,
                        children: [
                            { id: 'sh1', display: 'flex', width: 28, height: 28, color: C.tint },
                            { id: 'sh2', display: 'flex', width: 28, height: 28, color: C.tint },
                        ],
                    },
                ],
            },

            // Explicit height, so padding-top and a border are fair game here: the auto-height
            // double count in R8 only applies when the height has to be derived.
            {
                id: 'art_figure', display: 'block', height: 300, margin: { bottom: 24 },
                padding: { top: 12, right: 12, bottom: 12, left: 12 },
                border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.page,
                children: [
                    block('fig_img', 240, { margin: { bottom: 12 }, color: C.tint }),
                    block('fig_cap', 20, { color: C.mute }),
                ],
            },

            // AUTO height derived from six stacked children.
            {
                id: 'art_body', display: 'block', margin: { bottom: 24 },
                children: [
                    para('p1', 96, 14), para('p2', 72, 14), para('p3', 120, 14),
                    para('p4', 84, 14), para('p5', 108, 14), para('p6', 60, 0),
                ],
            },

            // Horizontal block margins plus a left border: the width is the content box less both
            // margins, and LocalX is padding-left plus margin-left.
            {
                id: 'art_pullquote', display: 'block', height: 120,
                margin: { left: 40, right: 40, bottom: 24 },
                padding: { left: 20 }, border: { left: 4 }, color: C.tint,
                children: [
                    block('pq_l1', 16, { margin: { bottom: 10 }, color: C.mute }),
                    block('pq_l2', 16, { color: C.mute }),
                ],
            },

            block('art_chips_label', 12, { margin: { bottom: 10 }, color: C.mute }),

            // Twelve inline-block boxes into 780px: the break after chip5, the two line heights
            // (32+8 and 30+8) and this container's derived height all come out of the line loop.
            {
                id: 'chips', display: 'block', width: 780, margin: { bottom: 24 },
                children: CHIPS.map((dims, i) => chip(i, dims)),
            },

            {
                id: 'art_related', display: 'flex', direction: 'row', gapColumn: 20, height: 180,
                children: [relatedCard(1), relatedCard(2), relatedCard(3)],
            },
        ],
    },
};
