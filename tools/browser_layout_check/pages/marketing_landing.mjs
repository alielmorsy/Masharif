// Full-page case: a marketing landing page (hero / features / logo wall / pricing / CTA / footer).
//
// What this covers that the small fixtures do not: two absolutely-positioned boxes sharing one
// containing block, one of them pinned on both horizontal insets with an AUTO width; a wrapping
// logo wall where `justify-content:space-between` and `align-content:space-between` both have to
// distribute leftovers on a line count nobody stated; and a pricing row where one card's
// `align-self:stretch` beats the container's `align-items:flex-start` while its siblings keep
// their own heights.
//
// The hero is split into an unpadded positioned shell plus a padded inner column on purpose:
// R12 -- the engine anchors absolute children at the containing block's content-box origin, CSS
// at its padding edge, so a containing block with padding could not agree with the browser about
// anything. Wrapping is the fix, not a workaround: it is how you write this layout such that both
// engines are answering the same question.

const C = {
    ink: '#141b2d', deep: '#1d2a4d', panel: '#ffffff', page: '#f6f7fb',
    line: '#dde1ea', mute: '#98a1b3', accent: '#3d5afe', warm: '#ff8a3d', good: '#1fae7a',
};

const bar = (id, w, h, color) => ({ id, display: 'flex', width: w, height: h, color });
const spacer = (id) => ({
    id, display: 'flex', flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0, minHeight: 0,
});
/// AUTO-width line: `align-items:stretch` fills the container's content box on the cross axis.
const line = (id, h, color) => ({ id, display: 'flex', height: h, color });

const featureCard = (i, iconColor) => ({
    id: `feat${i}`, display: 'flex', direction: 'column', gapRow: 12,
    flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
    padding: { top: 20, right: 20, bottom: 20, left: 20 },
    border: { top: 1, right: 1, bottom: 1, left: 1 }, color: C.panel,
    children: [
        bar(`feat${i}_icon`, 40, 40, iconColor),
        bar(`feat${i}_title`, '70%', 16, C.ink),
        line(`feat${i}_l1`, 10, C.line),
        line(`feat${i}_l2`, 10, C.line),
        bar(`feat${i}_l3`, '80%', 10, C.line),
        spacer(`feat${i}_gap`),
        bar(`feat${i}_link`, 90, 12, C.accent),
    ],
});

/// `featured` drops its own height and takes align-self:stretch instead, so the container's
/// align-items:flex-start applies to its two siblings and not to it.
const planCard = (i, { featured = false } = {}) => ({
    id: `plan${i}`, display: 'flex', direction: 'column', gapRow: 12,
    flexGrow: 1, flexShrink: 1, flexBasis: 0, minWidth: 0,
    ...(featured ? { alignSelf: 'stretch' } : { height: 240 }),
    padding: { top: 20, right: 20, bottom: 20, left: 20 },
    border: { top: 1, right: 1, bottom: 1, left: 1 },
    color: featured ? C.page : C.panel,
    children: [
        bar(`plan${i}_name`, 90, 14, C.mute),
        bar(`plan${i}_price`, 120, 32, C.ink),
        line(`plan${i}_f1`, 10, C.line),
        line(`plan${i}_f2`, 10, C.line),
        line(`plan${i}_f3`, 10, C.line),
        spacer(`plan${i}_gap`),
        line(`plan${i}_btn`, 40, featured ? C.accent : C.line),
    ],
});

const linkColumn = (i) => ({
    id: `fcol${i}`, display: 'flex', direction: 'column', gapRow: 8,
    width: 140, flexGrow: 0, flexShrink: 0,
    children: [
        line(`fcol${i}_title`, 12, C.mute),
        line(`fcol${i}_a`, 10, C.line),
        line(`fcol${i}_b`, 10, C.line),
        line(`fcol${i}_c`, 10, C.line),
    ],
});

export default {
    name: 'page_marketing_landing',
    title: 'Full page: marketing landing (hero overlay / feature grid / logo wall / pricing / CTA / footer)',
    calc: { w: 1200, h: 1400 },
    root: {
        id: 'root', display: 'flex', direction: 'column', width: 1200, height: 1400, color: C.page,
        children: [
            // ---------------------------------------------------------------- hero
            {
                // Unpadded and positioned: the containing block for the two absolute children.
                id: 'hero', display: 'flex', direction: 'column', position: 'relative',
                height: 380, flexGrow: 0, flexShrink: 0, color: C.deep,
                children: [
                    // AUTO width pinned on both horizontal insets -- the width is the inset gap,
                    // not a stated size.
                    {
                        id: 'hero_ribbon', display: 'flex', position: 'absolute',
                        offsets: { top: 0, left: 0, right: 0 }, height: 6, color: C.warm,
                    },
                    {
                        id: 'hero_badge', display: 'flex', position: 'absolute',
                        offsets: { top: 24, right: 24 }, width: 120, height: 28, color: C.accent,
                    },
                    {
                        id: 'hero_inner', display: 'flex', direction: 'column', justify: 'center', gapRow: 20,
                        flexGrow: 1, flexShrink: 1, flexBasis: 0, minHeight: 0,
                        padding: { top: 48, right: 48, bottom: 48, left: 48 },
                        children: [
                            bar('hero_title', '55%', 56, '#e8ecf6'),
                            bar('hero_sub', '45%', 40, C.mute),
                            {
                                id: 'hero_cta', display: 'flex', direction: 'row', gapColumn: 16,
                                height: 48, flexGrow: 0, flexShrink: 0,
                                children: [bar('cta_primary', 160, 48, C.accent), bar('cta_ghost', 140, 48, '#2b3a5e')],
                            },
                            {
                                // Three different heights under align-items:baseline. With no text
                                // anywhere, every box's baseline is synthesised from its bottom
                                // margin edge, so this is really "do the bottoms line up".
                                id: 'hero_stats', display: 'flex', direction: 'row', alignItems: 'baseline',
                                gapColumn: 32, height: 40, flexGrow: 0, flexShrink: 0,
                                children: [
                                    bar('hs1', 80, 40, '#2b3a5e'),
                                    bar('hs2', 80, 28, '#2b3a5e'),
                                    bar('hs3', 80, 34, '#2b3a5e'),
                                ],
                            },
                        ],
                    },
                ],
            },

            // ---------------------------------------------------------------- features
            {
                id: 'features', display: 'flex', direction: 'row', gapColumn: 24,
                height: 240, flexGrow: 0, flexShrink: 0, padding: { left: 48, right: 48 },
                children: [featureCard(1, C.accent), featureCard(2, C.good), featureCard(3, C.warm)],
            },

            // ---------------------------------------------------------------- logo wall
            {
                // 10 fixed items into 1120px of content width fit 6 to a line, so the line count,
                // the per-line free space and the cross-axis line packing are all derived, not stated.
                id: 'logos', display: 'flex', direction: 'row', wrap: 'wrap',
                justify: 'space-between', alignContent: 'space-between',
                gapRow: 16, gapColumn: 16, height: 200, flexGrow: 0, flexShrink: 0,
                padding: { top: 40, right: 40, bottom: 40, left: 40 }, color: C.panel,
                children: Array.from({ length: 10 }, (_, i) => bar(`logo${i}`, 160, 48, C.line)),
            },

            // ---------------------------------------------------------------- pricing
            {
                id: 'pricing', display: 'flex', direction: 'row', alignItems: 'flex-start', gapColumn: 24,
                height: 300, flexGrow: 0, flexShrink: 0, padding: { left: 48, right: 48 },
                children: [planCard(1), planCard(2, { featured: true }), planCard(3)],
            },

            // ---------------------------------------------------------------- CTA band
            {
                id: 'cta_band', display: 'flex', direction: 'row', justify: 'center', alignItems: 'center',
                gapColumn: 24, height: 120, flexGrow: 0, flexShrink: 0, color: C.ink,
                children: [bar('cb_text', '40%', 32, '#e8ecf6'), bar('cb_btn', 180, 48, C.accent)],
            },

            // ---------------------------------------------------------------- footer
            {
                id: 'footer', display: 'flex', direction: 'row', justify: 'space-between',
                height: 160, flexGrow: 0, flexShrink: 0, color: C.deep,
                padding: { top: 40, right: 40, bottom: 40, left: 40 },
                children: [
                    {
                        id: 'ft_brand', display: 'flex', direction: 'column', gapRow: 10,
                        width: 200, flexGrow: 0, flexShrink: 0,
                        children: [bar('fb_mark', 40, 40, C.accent), bar('fb_line', 160, 12, C.mute)],
                    },
                    linkColumn(1), linkColumn(2), linkColumn(3),
                ],
            },
        ],
    },
};
