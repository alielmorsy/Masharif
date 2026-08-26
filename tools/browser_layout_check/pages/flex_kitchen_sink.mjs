// Full-page case: one page that puts every flexbox feature the engine implements on screen at
// once, in the shape of a component gallery.
//
// Sections, and what each is the only place to see:
//   1  justify-content   -- all six values against the same free space
//   2  align-items       -- all five values, plus the AUTO-cross-size stretch case
//   3  align-content     -- all seven values over a derived line count
//   4  flex-direction    -- all four, including both reverses
//   5  flex-wrap         -- nowrap-with-shrink, wrap, wrap-reverse
//   6  flexibility       -- grow ratios, shrink ratios, all four basis kinds, min/max clamps on
//                           both axes (including a grow item that hits max-width and hands its
//                           surplus to a sibling -- the two-pass freeze)
//   7  spacing           -- margin:auto on both axes, percentage margin, percentage padding
//   8  out of flow       -- mixed insets, both-inset pinning, full stretch, auto-margin centring
//                           inside a fully-inset box, and the static position of an inset-less box
//   9  order / none / align-self -- reordering, display:none in flow and out of flow, per-item overrides
//  10  depth             -- six nested flex levels, each adding padding, border and gap, with
//                           percentage sizes at the bottom, plus an inline-flex item
//
// Section 5's wrap tiles pin `align-content:flex-start` on purpose: section 3 is where line
// packing is under test, and letting it float here would smear one align-content bug across two
// sections instead of localising it.

const C = {
    page: '#fbfbfd', tile: '#ffffff', line: '#dcdfe6', ink: '#2c3340', mute: '#98a1b3',
    a: '#3d5afe', b: '#1fae7a', c: '#ff8a3d', d: '#e0245e', e: '#8b5cf6',
};

const box = (id, w, h, color) => ({ id, display: 'flex', width: w, height: h, color });

/// A framed demo tile. Explicit width everywhere in this page: fractional flex division is
/// exercised on purpose in section 6, and everywhere else it would only add noise.
const tile = (id, w, h, extra = {}) => ({
    id, display: 'flex', width: w, height: h, flexGrow: 0, flexShrink: 0,
    padding: { top: 8, right: 8, bottom: 8, left: 8 },
    border: { top: 1, right: 1, bottom: 1, left: 1 },
    color: C.tile, ...extra,
});

const section = (id, height, extra = {}) => ({
    id, display: 'flex', direction: 'row', gapColumn: 12,
    height, flexGrow: 0, flexShrink: 0, ...extra,
});

// ---------------------------------------------------------------- 1. justify-content

const JUSTIFY = ['flex-start', 'flex-end', 'center', 'space-between', 'space-around', 'space-evenly'];
const justifyRow = (v, i) => ({
    id: `jr_${v.replace(/-/g, '_')}`, display: 'flex', direction: 'row', justify: v,
    alignItems: 'center', gapColumn: 8, height: 44, flexGrow: 0, flexShrink: 0, color: C.tile,
    children: [
        box(`jr${i}_a`, 120, 28, C.a), box(`jr${i}_b`, 120, 28, C.b), box(`jr${i}_c`, 120, 28, C.c),
    ],
});

// ---------------------------------------------------------------- 2. align-items

const ALIGN_ITEMS = ['stretch', 'flex-start', 'flex-end', 'center', 'baseline'];
const alignTile = (v, i) => tile(`ai_${v.replace(/-/g, '_')}`, 240, 120, {
    direction: 'row', alignItems: v, gapColumn: 8,
    children: v === 'stretch'
        // No cross size at all: the only items on the page whose height is the container's
        // decision rather than their own.
        ? [box(`ai${i}_a`, 60, undefined, C.a), box(`ai${i}_b`, 60, undefined, C.b), box(`ai${i}_c`, 60, undefined, C.c)]
        : [box(`ai${i}_a`, 60, 28, C.a), box(`ai${i}_b`, 60, 56, C.b), box(`ai${i}_c`, 60, 40, C.c)],
});

// ---------------------------------------------------------------- 3. align-content

const ALIGN_CONTENT = ['stretch', 'flex-start', 'flex-end', 'center', 'space-between', 'space-around', 'space-evenly'];
/// 154px of content width fits two 48px items per line, so five items make three lines -- a count
/// the spec never states and the packing then has 64px of slack to distribute.
const contentTile = (v, i) => tile(`ac_${v.replace(/-/g, '_')}`, 168, 180, {
    direction: 'row', wrap: 'wrap', alignContent: v, gapRow: 6, gapColumn: 6,
    children: Array.from({ length: 5 }, (_, k) => box(`ac${i}_${k}`, 48, 30, [C.a, C.b, C.c, C.d, C.e][k])),
});

// ---------------------------------------------------------------- 4. flex-direction

const DIRECTIONS = ['row', 'row-reverse', 'column', 'column-reverse'];
const directionTile = (v, i) => {
    const isRow = v.startsWith('row');
    return tile(`dir_${v.replace(/-/g, '_')}`, 200, 160, {
        direction: v, ...(isRow ? { gapColumn: 8 } : { gapRow: 8 }),
        children: [
            box(`dir${i}_1`, 48, isRow ? 48 : 36, C.a),
            box(`dir${i}_2`, 48, isRow ? 48 : 36, C.b),
            box(`dir${i}_3`, 48, isRow ? 48 : 36, C.c),
        ],
    });
};

// ---------------------------------------------------------------- 5. flex-wrap

const wrapTile = (v, i) => tile(`wrap_${v.replace(/-/g, '_')}`, 300, 180, {
    direction: 'row', wrap: v, alignContent: 'flex-start', gapRow: 6, gapColumn: 6,
    children: Array.from({ length: 5 }, (_, k) => ({
        // nowrap has 400px of basis in 258px of space: the deficit is shared equally because
        // every basis is equal, which is also the only place shrink is under test here.
        id: `wr${i}_${k}`, display: 'flex', width: 80, height: 40, flexShrink: 1,
        color: [C.a, C.b, C.c, C.d, C.e][k],
    })),
});

// ---------------------------------------------------------------- 6. flexibility

const fxRow = (id, children) => ({
    id, display: 'flex', direction: 'row', gapColumn: 10, height: 56,
    flexGrow: 0, flexShrink: 0, color: C.tile, children,
});

// ---------------------------------------------------------------- 8. out of flow

/// Every containing block here is unpadded and unbordered (R12): the engine anchors absolute
/// children at the content-box origin and measures percentage insets against the border box,
/// where CSS uses the padding box for both, so padding would put the two sides in different
/// coordinate systems before the interesting part even started.
const absTile = (id, children, extra = {}) => ({
    id, display: 'flex', position: 'relative', width: 300, height: 200,
    flexGrow: 0, flexShrink: 0, color: C.tile, ...extra, children,
});

export default {
    name: 'page_flex_kitchen_sink',
    title: 'Full page: every implemented flex feature in one gallery',
    note: 'Ten sections, all inside flex containers: justify, align-items, align-content, direction, wrap, grow/shrink/basis/clamps, auto+percent spacing, out-of-flow, order/none/self, and six-level nesting.',
    calc: { w: 1360, h: 2400 },
    root: {
        id: 'root', display: 'flex', direction: 'column', gapRow: 20,
        width: 1360, height: 2400, color: C.page,
        padding: { top: 40, right: 40, bottom: 40, left: 40 },
        children: [
            // 1 ----------------------------------------------------------- justify-content
            {
                id: 'sec_justify', display: 'flex', direction: 'column', gapRow: 10,
                height: 314, flexGrow: 0, flexShrink: 0,
                children: JUSTIFY.map(justifyRow),
            },

            // 2 ----------------------------------------------------------- align-items
            section('sec_align_items', 120, { children: ALIGN_ITEMS.map(alignTile) }),

            // 3 ----------------------------------------------------------- align-content
            section('sec_align_content', 180, { children: ALIGN_CONTENT.map(contentTile) }),

            // 4 ----------------------------------------------------------- flex-direction
            section('sec_direction', 160, { children: DIRECTIONS.map(directionTile) }),

            // 5 ----------------------------------------------------------- flex-wrap
            section('sec_wrap', 180, { children: ['nowrap', 'wrap', 'wrap-reverse'].map(wrapTile) }),

            // 6 ----------------------------------------------------------- flexibility
            {
                id: 'sec_flex', display: 'flex', direction: 'column', gapRow: 10,
                height: 320, flexGrow: 0, flexShrink: 0,
                children: [
                    // Grow ratios: 1260px of free space split 1:2:3 lands on exact integers.
                    fxRow('fx_grow', [
                        { id: 'g1', display: 'flex', flexGrow: 1, flexBasis: 0, minWidth: 0, height: 36, color: C.a },
                        { id: 'g2', display: 'flex', flexGrow: 2, flexBasis: 0, minWidth: 0, height: 36, color: C.b },
                        { id: 'g3', display: 'flex', flexGrow: 3, flexBasis: 0, minWidth: 0, height: 36, color: C.c },
                    ]),
                    // Shrink ratios: 1540px of basis into 1280px, weighted by basis x shrink.
                    fxRow('fx_shrink', [
                        { id: 's1', display: 'flex', width: 500, flexShrink: 1, height: 36, color: C.a },
                        { id: 's2', display: 'flex', width: 500, flexShrink: 2, height: 36, color: C.b },
                        { id: 's3', display: 'flex', width: 500, flexShrink: 3, height: 36, color: C.c },
                    ]),
                    // All four basis kinds side by side; the last one absorbs what the others left.
                    fxRow('fx_basis', [
                        { id: 'b_px', display: 'flex', flexBasis: 120, height: 36, color: C.a },
                        { id: 'b_pct', display: 'flex', flexBasis: '20%', height: 36, color: C.b },
                        { id: 'b_auto', display: 'flex', flexBasis: 'auto', width: 90, height: 36, color: C.c },
                        { id: 'b_zero', display: 'flex', flexBasis: 0, flexGrow: 1, minWidth: 0, height: 36, color: C.d },
                    ]),
                    // c_max freezes at 140 and hands its surplus to c_grow -- the two-pass
                    // grow resolution, not a single proportional division.
                    fxRow('fx_clamp_main', [
                        { id: 'c_max', display: 'flex', flexBasis: 0, flexGrow: 1, maxWidth: 140, height: 36, color: C.a },
                        { id: 'c_min', display: 'flex', width: 320, flexShrink: 1, minWidth: 200, height: 36, color: C.b },
                        { id: 'c_grow', display: 'flex', flexBasis: 0, flexGrow: 1, minWidth: 0, height: 36, color: C.c },
                    ]),
                    // Cross-axis clamps: one stretch capped by max-height, one fixed height
                    // pushed up by min-height.
                    fxRow('fx_clamp_cross', [
                        { id: 'x_stretch_max', display: 'flex', width: 200, alignSelf: 'stretch', maxHeight: 30, color: C.a },
                        { id: 'x_min_up', display: 'flex', width: 200, height: 20, minHeight: 48, color: C.b },
                        { id: 'x_plain', display: 'flex', width: 200, height: 36, color: C.c },
                    ]),
                ],
            },

            // 7 ----------------------------------------------------------- spacing
            section('sec_spacing', 180, {
                children: [
                    // margin:auto on both axes absorbs all free space on both axes.
                    tile('sp_auto_both', 300, 160, {
                        direction: 'row', padding: {}, border: {},
                        children: [{
                            id: 'sp_center', display: 'flex', width: 100, height: 60, color: C.a,
                            margin: { top: 'auto', right: 'auto', bottom: 'auto', left: 'auto' },
                        }],
                    }),
                    // A single auto margin between two items: all free space collects there.
                    tile('sp_auto_gap', 300, 160, {
                        direction: 'row', alignItems: 'center', padding: {}, border: {},
                        children: [
                            { id: 'sp_left', display: 'flex', width: 70, height: 50, color: C.b, margin: { right: 'auto' } },
                            box('sp_right', 70, 50, C.c),
                        ],
                    }),
                    // Percentage margins resolve against the container's width on BOTH axes --
                    // top:5% of 300 is 15px, not 5% of 160.
                    tile('sp_pct_margin', 300, 160, {
                        direction: 'row', padding: {}, border: {},
                        children: [{
                            id: 'sp_pm', display: 'flex', width: 100, height: 60, color: C.d,
                            margin: { left: '10%', top: '5%' },
                        }],
                    }),
                    // Percentage padding, same width reference. The child's own rect is unchanged
                    // by it; `sp_pp_inner` is what makes it observable.
                    tile('sp_pct_padding', 300, 160, {
                        direction: 'row', padding: {}, border: {},
                        children: [{
                            id: 'sp_pp', display: 'flex', direction: 'column', width: 120, height: 80, color: C.e,
                            padding: { left: '10%', top: '10%' },
                            children: [{ id: 'sp_pp_inner', display: 'flex', height: 20, color: C.tile }],
                        }],
                    }),
                ],
            }),

            // 8 ----------------------------------------------------------- out of flow
            section('sec_absolute', 200, {
                children: [
                    absTile('ab_mixed', [
                        { id: 'ab_tl', display: 'flex', position: 'absolute', offsets: { top: 10, left: 10 }, width: 60, height: 40, color: C.a },
                        { id: 'ab_br', display: 'flex', position: 'absolute', offsets: { bottom: 10, right: 10 }, width: 60, height: 40, color: C.b },
                        // No width or height: both axes are the inset gap.
                        { id: 'ab_inset', display: 'flex', position: 'absolute', offsets: { top: '25%', left: '25%', right: '25%', bottom: '25%' }, color: C.c },
                    ]),
                    absTile('ab_bar', [
                        {
                            id: 'ab_bar_inner', display: 'flex', position: 'absolute',
                            offsets: { left: 0, right: 0, top: 12 }, height: 40,
                            direction: 'row', justify: 'center', alignItems: 'center', color: C.line,
                            children: [box('ab_pill', 60, 28, C.d)],
                        },
                        box('ab_bar_flow', 80, 40, C.mute),
                    ]),
                    absTile('ab_fill', [
                        {
                            id: 'ab_fill_inner', display: 'flex', position: 'absolute',
                            offsets: { top: 0, right: 0, bottom: 0, left: 0 },
                            direction: 'column', justify: 'flex-end', color: C.line,
                            children: [box('ab_fill_child', 100, 30, C.e)],
                        },
                    ]),
                    absTile('ab_static', [
                        // Fully inset AND explicitly sized, with auto margins: over-constrained,
                        // so the auto margins centre it in the inset box.
                        {
                            id: 'ab_auto_centre', display: 'flex', position: 'absolute',
                            offsets: { top: 0, right: 0, bottom: 0, left: 0 }, width: 120, height: 60,
                            margin: { top: 'auto', right: 'auto', bottom: 'auto', left: 'auto' }, color: C.a,
                        },
                        // No insets at all: it lands at its static position, i.e. where the
                        // container's justify/align would have put it in flow.
                        { id: 'ab_nostatic', display: 'flex', position: 'absolute', width: 50, height: 24, color: C.d },
                    ], { justify: 'center', alignItems: 'flex-end' }),
                ],
            }),

            // 9 ----------------------------------------------------------- order / none / self
            section('sec_order', 140, {
                children: [
                    tile('ord', 400, 120, {
                        direction: 'row', alignItems: 'center', gapColumn: 8,
                        children: [
                            { id: 'ord_dom1', display: 'flex', order: 3, width: 60, height: 40, color: C.a },
                            { id: 'ord_dom2', display: 'flex', order: 1, width: 70, height: 40, color: C.b },
                            { id: 'ord_dom3', display: 'flex', order: 4, width: 80, height: 40, color: C.c },
                            { id: 'ord_dom4', display: 'flex', order: 2, width: 90, height: 40, color: C.d },
                        ],
                    }),
                    tile('none_tile', 400, 120, {
                        direction: 'row', alignItems: 'center', gapColumn: 8, position: 'relative',
                        padding: {}, border: {},
                        children: [
                            box('nt_a', 60, 40, C.a),
                            // Generates no box: takes no flow space and contributes no gap.
                            { id: 'nt_hidden', display: 'none', width: 60, height: 40, color: C.d },
                            box('nt_c', 60, 40, C.b),
                            { id: 'nt_abs_hidden', display: 'none', position: 'absolute', offsets: { top: 10, left: 10 }, width: 20, height: 20, color: C.d },
                        ],
                    }),
                    tile('selfs', 400, 120, {
                        direction: 'row', alignItems: 'flex-start', gapColumn: 8,
                        children: [
                            box('sf_auto', 60, 36, C.a),
                            { id: 'sf_end', display: 'flex', width: 60, height: 36, alignSelf: 'flex-end', color: C.b },
                            { id: 'sf_center', display: 'flex', width: 60, height: 36, alignSelf: 'center', color: C.c },
                            { id: 'sf_stretch', display: 'flex', width: 60, alignSelf: 'stretch', color: C.d },
                            { id: 'sf_baseline', display: 'flex', width: 60, height: 52, alignSelf: 'baseline', color: C.e },
                        ],
                    }),
                ],
            }),

            // 10 ---------------------------------------------------------- depth
            section('sec_deep', 200, {
                children: [
                    {
                        id: 'd1', display: 'flex', direction: 'row', gapColumn: 8,
                        width: 600, height: 200, flexGrow: 0, flexShrink: 0,
                        padding: { top: 8, right: 8, bottom: 8, left: 8 },
                        border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.tile,
                        children: [
                            {
                                id: 'd2', display: 'flex', direction: 'column', gapRow: 8,
                                flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
                                padding: { top: 8, right: 8, bottom: 8, left: 8 },
                                border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.page,
                                children: [
                                    {
                                        id: 'd3', display: 'flex', direction: 'row', gapColumn: 6,
                                        flexGrow: 1, flexShrink: 1, flexBasis: 0, minHeight: 0,
                                        padding: { top: 6, right: 6, bottom: 6, left: 6 },
                                        border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.tile,
                                        children: [
                                            {
                                                id: 'd4', display: 'flex', direction: 'column', gapRow: 6,
                                                flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
                                                padding: { top: 6, right: 6, bottom: 6, left: 6 },
                                                border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.page,
                                                children: [
                                                    {
                                                        id: 'd5', display: 'flex', direction: 'row',
                                                        justify: 'space-evenly', alignItems: 'center',
                                                        flexGrow: 1, flexShrink: 1, flexBasis: 0, minHeight: 0,
                                                        padding: { top: 4, right: 4, bottom: 4, left: 4 },
                                                        border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.tile,
                                                        children: [
                                                            // Percentages six levels down: the
                                                            // reference box is whatever the five
                                                            // enclosing solves left behind.
                                                            box('d6_pct_a', '30%', '50%', C.a),
                                                            box('d6_px', 40, 20, C.b),
                                                            box('d6_pct_b', '20%', '40%', C.c),
                                                        ],
                                                    },
                                                ],
                                            },
                                        ],
                                    },
                                ],
                            },
                            // inline-flex as a flex item: CSS blockifies it, and the engine routes
                            // it to the flex strategy the same way (LayoutStrategy.cpp:11).
                            {
                                id: 'd1_inline', display: 'inline-flex', direction: 'column',
                                justify: 'space-between', width: 120, flexGrow: 0, flexShrink: 0,
                                padding: { top: 6, right: 6, bottom: 6, left: 6 }, color: C.page,
                                children: [box('d1_in_top', 40, 24, C.d), box('d1_in_bot', 40, 24, C.e)],
                            },
                        ],
                    },
                ],
            }),
        ],
    },
};
