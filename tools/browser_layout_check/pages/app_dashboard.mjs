// Full-page case: an application shell (top bar / sidebar / content / footer).
//
// What this covers that the small fixtures do not: a flex-grow chain that has to survive four
// levels of nesting (root column -> shell row -> main column -> panels row -> chart column),
// every level carrying `min-height:0`/`min-width:0` so the grow/shrink resolution actually
// reaches the leaves; an absolutely-positioned badge whose containing block is a nested
// `position:relative` flex item rather than the root; and percentage heights resolved against
// a container whose own height was itself decided by flex distribution three levels up.
//
// Sizes are chosen so the flex divisions land on integers (992 content width -> 4 x 236 + 3 x 16
// gaps; 972 -> 648 + 324 at 2:1), keeping subpixel noise out of everything except the chart
// bars, which are deliberately left indivisible to exercise fractional distribution.

const C = {
    page: '#f4f5f7', bar: '#1f2933', panel: '#ffffff', line: '#d9dee4',
    ink: '#3e4c59', mute: '#9aa5b1', accent: '#2f6feb', warm: '#f0a132', good: '#3aa76d',
};

/// A zero-basis flex item whose only job is to eat the remaining space on the main axis.
const spacer = (id, cross) => ({
    id, display: 'flex', flexGrow: 1, flexShrink: 1, flexBasis: 0,
    minWidth: 0, minHeight: 0, ...(cross ?? {}),
});

const bar = (id, w, h, color) => ({ id, display: 'flex', width: w, height: h, color });

/// One of the four KPI cards: width comes entirely from flex distribution (basis 0 + grow),
/// so its padding and border inset a box the solver sized, not one the spec stated.
const statCard = (i, color) => ({
    id: `stat${i}`, display: 'flex', direction: 'column', gapRow: 10,
    flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
    padding: { top: 16, right: 16, bottom: 16, left: 16 },
    border: { top: 1, right: 1, bottom: 1, left: 1 },
    color: C.panel,
    children: [
        // No width: an AUTO-width flex item under the default `align-items:stretch`, so the
        // cross axis is filled by the container's content box on both sides.
        { id: `stat${i}_label`, display: 'flex', height: 12, color: C.mute },
        bar(`stat${i}_value`, 80, 24, color),
    ],
});

/// A list row: fixed-size avatar, an AUTO-width text column that absorbs the remainder, and a
/// fixed trailing pill. The inner `60%` bar resolves against a width the solver produced.
const listRow = (i) => ({
    id: `row${i}`, display: 'flex', direction: 'row', alignItems: 'center', gapColumn: 10,
    height: 44, flexGrow: 0, flexShrink: 0, color: C.page,
    children: [
        bar(`row${i}_avatar`, 28, 28, C.accent),
        {
            id: `row${i}_text`, display: 'flex', direction: 'column', gapRow: 6,
            flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
            children: [
                { id: `row${i}_t1`, display: 'flex', height: 10, color: C.ink },
                { id: `row${i}_t2`, display: 'flex', width: '60%', height: 8, color: C.mute },
            ],
        },
        bar(`row${i}_pill`, 48, 18, C.good),
    ],
});

const sideGroup = (i, count) => ({
    id: `side${i}`, display: 'flex', direction: 'column', gapRow: 8,
    flexGrow: 0, flexShrink: 0,
    children: [
        bar(`side${i}_title`, 80, 12, C.mute),
        ...Array.from({ length: count }, (_, k) => ({
            id: `side${i}_item${k}`, display: 'flex', height: 32, color: '#2b3a47',
        })),
    ],
});

const BAR_HEIGHTS = ['30%', '55%', '40%', '80%', '65%', '95%', '50%', '70%'];

export default {
    name: 'page_app_dashboard',
    title: 'Full page: application shell (top bar / sidebar / content panels / footer)',
    calc: { w: 1280, h: 860 },
    root: {
        id: 'root', display: 'flex', direction: 'column', width: 1280, height: 860, color: C.page,
        children: [
            // ---------------------------------------------------------------- top bar
            {
                id: 'topbar', display: 'flex', direction: 'row', alignItems: 'center', gapColumn: 16,
                height: 56, flexGrow: 0, flexShrink: 0, color: C.bar,
                padding: { left: 20, right: 20 },
                children: [
                    {
                        id: 'brand', display: 'flex', direction: 'row', alignItems: 'center', gapColumn: 8,
                        width: 180, height: 32, flexGrow: 0, flexShrink: 0,
                        children: [bar('brand_mark', 28, 28, C.accent), bar('brand_text', 120, 14, '#e4e7eb')],
                    },
                    {
                        id: 'topnav', display: 'flex', direction: 'row', alignItems: 'center', gapColumn: 20,
                        height: 32, flexGrow: 0, flexShrink: 0,
                        children: [
                            bar('nav_a', 64, 14, C.mute), bar('nav_b', 72, 14, C.mute), bar('nav_c', 56, 14, C.mute),
                        ],
                    },
                    spacer('topbar_gap', { height: 1 }),
                    {
                        id: 'actions', display: 'flex', direction: 'row', alignItems: 'center', gapColumn: 12,
                        height: 32, flexGrow: 0, flexShrink: 0,
                        children: [
                            bar('search', 220, 32, '#323f4b'),
                            // position:relative with no offsets -- purely a containing block for
                            // `badge`, which is what makes the absolute case interesting: the
                            // nearest positioned ancestor is a flex item four levels down, not root.
                            {
                                id: 'bell_wrap', display: 'flex', position: 'relative',
                                justify: 'center', alignItems: 'center', width: 32, height: 32,
                                flexGrow: 0, flexShrink: 0, color: '#323f4b',
                                children: [
                                    bar('bell', 20, 20, C.mute),
                                    {
                                        id: 'badge', display: 'flex', position: 'absolute',
                                        offsets: { top: -4, right: -4 }, width: 16, height: 16, color: '#e12d39',
                                    },
                                ],
                            },
                            bar('avatar', 32, 32, C.warm),
                        ],
                    },
                ],
            },

            // ---------------------------------------------------------------- shell
            {
                id: 'shell', display: 'flex', direction: 'row',
                flexGrow: 1, flexShrink: 1, flexBasis: 0, minHeight: 0,
                children: [
                    {
                        id: 'sidebar', display: 'flex', direction: 'column', gapRow: 20,
                        width: 240, flexGrow: 0, flexShrink: 0, color: C.bar,
                        padding: { top: 16, right: 16, bottom: 16, left: 16 },
                        border: { right: 1 },
                        children: [
                            sideGroup(1, 3),
                            sideGroup(2, 4),
                            spacer('side_gap'),
                            {
                                id: 'side_footer', display: 'flex', direction: 'row', alignItems: 'center',
                                gapColumn: 8, height: 40, flexGrow: 0, flexShrink: 0,
                                children: [
                                    bar('sf_avatar', 28, 28, C.good),
                                    {
                                        id: 'sf_text', display: 'flex', height: 12,
                                        flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0, color: C.mute,
                                    },
                                ],
                            },
                        ],
                    },
                    {
                        id: 'main', display: 'flex', direction: 'column', gapRow: 20,
                        flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
                        padding: { top: 24, right: 24, bottom: 24, left: 24 },
                        children: [
                            {
                                id: 'crumbs', display: 'flex', direction: 'row', alignItems: 'center', gapColumn: 8,
                                height: 16, flexGrow: 0, flexShrink: 0,
                                children: [
                                    bar('cr1', 60, 12, C.mute), bar('cr_s1', 6, 6, C.line),
                                    bar('cr2', 80, 12, C.mute), bar('cr_s2', 6, 6, C.line),
                                    bar('cr3', 100, 12, C.ink),
                                ],
                            },
                            {
                                id: 'stats', display: 'flex', direction: 'row', gapColumn: 16,
                                height: 96, flexGrow: 0, flexShrink: 0,
                                children: [
                                    statCard(1, C.accent), statCard(2, C.good),
                                    statCard(3, C.warm), statCard(4, '#e12d39'),
                                ],
                            },
                            {
                                id: 'panels', display: 'flex', direction: 'row', gapColumn: 20,
                                flexGrow: 1, flexShrink: 1, flexBasis: 0, minHeight: 0,
                                children: [
                                    {
                                        id: 'chart_panel', display: 'flex', direction: 'column', gapRow: 12,
                                        flexGrow: 2, flexShrink: 1, flexBasis: 0, minWidth: 0,
                                        padding: { top: 16, right: 16, bottom: 16, left: 16 },
                                        border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.panel,
                                        children: [
                                            {
                                                id: 'chart_head', display: 'flex', direction: 'row',
                                                justify: 'space-between', alignItems: 'center',
                                                height: 20, flexGrow: 0, flexShrink: 0,
                                                children: [
                                                    bar('ch_title', 120, 14, C.ink),
                                                    {
                                                        id: 'ch_legend', display: 'flex', direction: 'row', gapColumn: 10,
                                                        flexGrow: 0, flexShrink: 0,
                                                        children: [bar('lg1', 40, 10, C.accent), bar('lg2', 40, 10, C.warm)],
                                                    },
                                                ],
                                            },
                                            {
                                                // Percentage heights below resolve against this box's
                                                // content height, which no spec states: it is what the
                                                // grow chain left over, four containers down.
                                                id: 'chart_body', display: 'flex', direction: 'row',
                                                alignItems: 'flex-end', gapColumn: 8,
                                                flexGrow: 1, flexShrink: 1, flexBasis: 0, minHeight: 0,
                                                children: BAR_HEIGHTS.map((h, i) => ({
                                                    id: `cbar${i}`, display: 'flex', height: h,
                                                    flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
                                                    color: i % 2 ? C.warm : C.accent,
                                                })),
                                            },
                                        ],
                                    },
                                    {
                                        id: 'list_panel', display: 'flex', direction: 'column', gapRow: 10,
                                        flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
                                        padding: { top: 16, right: 16, bottom: 16, left: 16 },
                                        border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.panel,
                                        children: [
                                            bar('lp_title', 100, 14, C.ink),
                                            listRow(1), listRow(2), listRow(3), listRow(4), listRow(5),
                                            spacer('lp_gap'),
                                        ],
                                    },
                                ],
                            },
                        ],
                    },
                ],
            },

            // ---------------------------------------------------------------- footer
            {
                id: 'footer', display: 'flex', direction: 'row', alignItems: 'center',
                justify: 'space-between', height: 48, flexGrow: 0, flexShrink: 0,
                padding: { left: 24, right: 24 }, border: { top: 1 }, color: C.panel,
                children: [
                    bar('ft_left', 200, 12, C.mute),
                    {
                        id: 'ft_right', display: 'flex', direction: 'row', gapColumn: 16,
                        flexGrow: 0, flexShrink: 0,
                        children: [bar('fr1', 60, 12, C.mute), bar('fr2', 60, 12, C.mute), bar('fr3', 60, 12, C.mute)],
                    },
                ],
            },
        ],
    },
};
